const functions = require('firebase-functions');

// The Firebase Admin SDK to access the Firebase Realtime Database.
const admin = require('firebase-admin');
admin.initializeApp();

//exports.scheduledFunction = functions.pubsub.schedule('every 5 minutes').onRun((context) => {
  //console.log('This will be run every 5 minutes!');
  //return null;
//});

const tagCoefficientWeight = 0.5;
const usersCoefficientWeight = 1 - tagCoefficientWeight;
const tagWeight = 0.5;
const usersWeight = 1 - tagWeight;
const tagNecessityForMax = 0.5;
const userTagNecessityForMax = 0.3;

const autofill = true;


function wait(ms){
   var start = new Date().getTime();
   var end = start;
   while(end < start + ms) {
      end = new Date().getTime();
   }
   return null;
}

function clamp(num, min, max) {
    return num <= min ? min : num >= max ? max : num;
}



function getCounterFromCollection(name){
	switch(name){
	case 'weight_list_users'	        :               return 'weight_list_counter';
	case 'tags' 			            :               return 'tagcount';
	case 'interested_posts_id_list'     :               return 'interestedInNumber';
	case 'interested'                   : if(autofill)  return 'interestedInNumber';    else break;
	case 'going_posts_id_list' 	        :               return 'goingToNumber';
	case 'going' 	                    : if(autofill)  return 'goingToNumber';         else break;
	}
	console.log('UNKOWN COLLECTION');
	return null;
}

// Keeps track of the length of the 'interested' child list in a separate property.

exports.countinterestedchange = functions.database.ref('/{category}/{id}/{collection}/{changedelement}').onWrite(
    async (change) => {

      const collectionKey = change.after.ref.parent.key;
      const collectionRef = change.after.ref.parent;
      const countKey = getCounterFromCollection(collectionKey);

      if(countKey===null){
	console.log('WENT VERY BAD, COULDN T RESOLVE FOR ' + collectionRef); 
	return null;
	}

      const countRef = collectionRef.parent.child(countKey);
	

      let increment;
      if (change.after.exists() && !change.before.exists()) {
        increment = 1;
      } else if (!change.after.exists() && change.before.exists()) {
        increment = -1;
      } else {
        return null;
      }

      // Return the promise from countRef.transaction() so our function
      // waits for this async event to complete before it exits.
      await countRef.transaction((current) => {
        return (current || 0) + increment;
      });
      console.log('Counter updated.');
      return null;
    });

function doesTagExist(taglist, tag){
    var ok = false;
    taglist.forEach(function(fval){
        if(fval.child('title').val() === tag){
            ok = true;
        }
    });
    return ok;
}


//calculates jaccard index 2 users
function su(user1, user2){
    console.log('calculating ' + user1.key + ' which has ' + user1.child('interested_posts_id_list').numChildren()  + ' and ' + user2.key + ' which has ' + user2.child('interested_posts_id_list').numChildren());

    var userBasedWeight;
    var tagBasedWeight;
    console.log('USER 1 HAS '  + user1.child('interested_posts_id_list').numChildren() + ' interested');
	var cnt = 0;
	user1.child('interested_posts_id_list').forEach(function(childNode){
		if(user2.child('interested_posts_id_list').child(childNode.key).exists()){
			cnt++;
		}	
	});
	console.log('found ' + cnt + ' similar items');
	userBasedWeight = Number(       Number(cnt)/(   Number(user1.child('interestedInNumber').val()) + Number(user2.child('interestedInNumber').val()) -  Number(cnt)  ) );
	if(isNaN(userBasedWeight)) {
	    userBasedWeight = Number(0);
	}
	userBasedWeight = clamp(userBasedWeight, 0, 1);
	console.log('USER BASED WEIGHT ' + userBasedWeight);

	cnt = 0;
	console.log('USER 1 HAS '  + user1.child('tags').numChildren() + ' tags');
	console.log('USER 1 HAS '  + user1.child('tagcount').val() + ' tags');

	console.log('USER 2 HAS '  + user2.child('tags').numChildren() + ' tags');
	console.log('USER 2 HAS '  + user2.child('tagcount').val() + ' tags');
	user1.child('tags').forEach(function(childNode){
	    if(doesTagExist(user2.child('tags'), childNode.child('title').val())){
	        console.log(user2.child('tags') + ' AAAND ' +  childNode.child('title').val());
	        cnt++;
	    }	
	});
	
	tagBasedWeight = Number(       (Number(cnt)/(   Number(user1.child('tagcount').val()) + Number(user2.child('tagcount').val()) -  Number(cnt)  )) / userTagNecessityForMax  );
	if(isNaN(tagBasedWeight)){
	    tagBasedWeight = Number(0);
	}
	tagBasedWeight = clamp(tagBasedWeight, 0, 1);

	console.log('TAG BASED WEIGHT ' + tagBasedWeight + ' FROM ' + cnt);
	return clamp(userBasedWeight * usersWeight + tagBasedWeight * tagWeight, 0, 1);
}

function csu(user, post){
    //averege with weights
	var incr = 0;
	var weightincr = 0;
	var val;
	user.child('weight_list_users').forEach(function(childNode){
		if(childNode.key !== user.key){
			val = Number(post.child('interested').child(childNode.key).exists());
		
			incr += Number(childNode.val()*val);
			weightincr += Number(childNode.val());
		}
	});
	if(Number(weightincr)===0) return 0;
	if(isNaN(Number(incr)/Number(weightincr))) return 0;
	return clamp(Number(incr)/Number(weightincr),0,1);
}

function cst(user, post){
	//tag requirement
	var cnt = 0;
	user.child('tags').forEach(function(childNode){
	    if(doesTagExist(post.child('tags'), childNode.child('title').val())){
			cnt++;
		}	
	});
	if(post.child('tagcount').val()===0){
	    return 0;
	}
	if(isNaN(Number(cnt)/(Number(post.child('tagcount').val())*tagNecessityForMax))) {
	    return 0;
	}
	var ret = Number(cnt)/(Number(post.child('tagcount').val())*tagNecessityForMax);
	return  clamp(ret, 0, 1);
}


exports.reciprocalListFill = functions.database.ref('/{category}/{id}/{collection}/{changedelement}').onWrite(async(change)=> {
    console.log('AUTODFFFIIIIIILLLLLLLLL!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! ' + autofill);
    if(autofill){
        const collectionKey = change.after.ref.parent.key;
        var reciprocalCollectionKey;
        if(collectionKey === 'interested_posts_id_list'){
            reciprocalCollectionKey = 'interested';
        }
        else if(collectionKey === 'going_posts_id_list'){
            reciprocalCollectionKey = 'going';
        } else{
            return null;
        }
        const collectionRef = change.after.ref.parent;
        const userRef = change.after.ref.parent.parent;
        const elementRef = change.after.ref;
        const baseref = change.after.ref.root;
        console.log('collectionKey is ' + collectionKey);
        console.log('collectionRef is ' + collectionRef.key);
        console.log('userRef  is ' + userRef.key );
        console.log('elementRef is ' + elementRef.key);
        if(userRef.parent.key === 'Users'){
            console.log('IT GOT HERE AND WE HAVE ' + change.before.exists() + ' ' + change.after.exists());
            var a;
            if (change.after.exists() && !change.before.exists()) {
                //added
                console.log('IT GOT HERE1');
                a = baseref.child('posts').child(elementRef.key).child(reciprocalCollectionKey).child(userRef.key).set(true);
            } else if (!change.after.exists() && change.before.exists()) {
                console.log('RMEWVED');
                a = baseref.child('posts').child(elementRef.key).child(reciprocalCollectionKey).child(userRef.key).remove();
            }

        }
    }
	return null; 

});


function updateUserCoefsUser(baseref, user, value){
    value.child('Users').forEach(function(childNode){
        if(childNode.key !== user.key){
            var coef = Number(su(user, childNode));
            console.log('ISSS ' + user.key + ' A CHILDDD???');
            console.log(' ' + childNode.key + ' and ' + user.key + ' resulted ' + coef);
            if(isNaN(coef)){
                coef = Number(0);
            }
            var x = baseref.child('Users').child(user.key).child('weight_list_users').child(childNode.key).set(coef);
            var y = baseref.child('Users').child(childNode.key).child('weight_list_users').child(user.key).set(coef);
        }
        return null;
    });
    return null;
}

function updatePostCoefsAll(baseref, value){

    value.child('Users').forEach(function(userChildNode){
        value.child('posts').forEach(function(postChildNode){
            var coeft = cst(userChildNode, postChildNode);
            var coefu = csu(userChildNode, postChildNode);
            //console.log('coeft for user ' + userChildNode.key + ' for the post ' + postChildNode.key + ' is ' + coeft);
            //console.log('coefu for user ' + userChildNode.key + ' for the post ' + postChildNode.key + ' is ' + coefu);
            var x = baseref.child('Users').child(userChildNode.key).child('post_coef_list').child(postChildNode.key).set(coeft * tagCoefficientWeight + coefu * usersCoefficientWeight);
        });
    });
}

function updatePostCoefsPost(baseref, post, value){

    value.child('Users').forEach(function(userChildNode){

            var coeft = cst(userChildNode, post);
            var coefu = csu(userChildNode, post);
            //console.log('coeft for user ' + userChildNode.key + ' for the post ' + postChildNode.key + ' is ' + coeft);
            //console.log('coefu for user ' + userChildNode.key + ' for the post ' + postChildNode.key + ' is ' + coefu);
            var x = baseref.child('Users').child(userChildNode.key).child('post_coef_list').child(post.key).set(coeft * tagCoefficientWeight + coefu * usersCoefficientWeight);
    });
}



//Someone got added to the going or interested list of this post
exports.updateWeightsOnListAddTrigger = functions.database.ref('/{category}/{id}/{collection}/{changedelement}').onWrite(
    async(change)=> {

    if(autofill) wait(100);//make sure counters are updated first
    var changedUserKey;
    var collectionRef = change.after.ref.parent;
    const baseref = change.after.ref.root;
    if(change.after.ref.parent.parent.parent.key==='Users'){
        console.log('USER: ' + change.after.key);
         
        changedUserKey = collectionRef.parent.key;
        if(collectionRef.key === 'tags'){
            return baseref.once('value').then(function(value){

                updateUserCoefsUser(baseref, value.child('Users').child(changedUserKey), value);
                updatePostCoefsAll(baseref, value);

                return;
            });
        }
    }
    if(change.after.ref.parent.parent.parent.key ==='posts'){
    	console.log('POST: ' + change.after.key);
 
    	changedUserKey = change.after.key;

	    if(collectionRef.key === 'interested' || collectionRef.key === 'going'){

		    return baseref.once('value').then(function(value){

		        updateUserCoefsUser(baseref, value.child('Users').child(changedUserKey), value);
		        updatePostCoefs(baseref, value);

			    return;
		    });
	    }
    }
    return null;
});


exports.updateWeightsOnIdAddTrigger = functions.database.ref('/{category}/{id}').onWrite(
    async(change)=> {

    if(autofill) wait(100);//make sure counters are updated first
    const baseref = change.after.ref.root;
    var categoryRef = change.after.ref.parent;
    var itemRef = change.after.ref;
    if(categoryRef.key==='Users'){
        console.log('USER ADDED: ' + change.after.key);
        if (change.after.exists() && !change.before.exists()) {
            var user = change.after;
            return baseref.once('value').then(function(value){
    	    
                updateUserCoefsUser(baseref,user,value);

                updatePostCoefsAll(baseref, value);
                return;
            });
        }
    }else if (!change.after.exists() && change.before.exists()) {
        console.log('USE REMOVED');

    }
    if(categoryRef.key==='posts'){
        var post = change.after;
        console.log('POST ADDED: ' + change.after.key);
        if (change.after.exists() && !change.before.exists()) {
            
            return baseref.once('value').then(function(value){

                updatePostCoefsPost(baseref, post, value);

                return;
            });
        } else if (!change.after.exists() && change.before.exists()) {
            return baseref.once('value').then(function(value){
                value.child('Users').forEach(function(userChildNode){
                    baseref.child('Users').child(userChildNode.key).child('post_coef_list').child(change.before.key).remove();
                });
                return;
            });
        } else {
            return null;
        }
    }
    return null;
});