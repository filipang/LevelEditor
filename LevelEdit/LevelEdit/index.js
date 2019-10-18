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

const autofill = false;


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
        console.log('THIS SHOULD HAPPEN FIRST');
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
          if(!current && increment === -1) return null;
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

    var userBasedWeight;
    var tagBasedWeight;
    var userIntCount1 = user1.child('interestedInNumber').exists() ? user1.child('interestedInNumber').val() : 0;
    var userIntCount2 = user2.child('interestedInNumber').exists() ? user2.child('interestedInNumber').val() : 0;
    
    var userTagCount1 = user1.child('tagcount').exists() ? user1.child('tagcount').val() : 0;
    var userTagCount2 = user2.child('tagcount').exists() ? user2.child('tagcount').val() : 0;
	var cnt = 0;
	user1.child('interested_posts_id_list').forEach(function(childNode){
		if(user2.child('interested_posts_id_list').child(childNode.key).exists()){
			cnt++;
		}	
	});

	userBasedWeight = Number(cnt) / (Number(userIntCount1) + Number(userIntCount2) - Number(cnt));
	if(isNaN(userBasedWeight)) {
	    userBasedWeight = Number(0);
	}
	userBasedWeight = clamp(userBasedWeight, 0, 1);

	cnt = 0;

	user1.child('tags').forEach(function(childNode){
	    if(doesTagExist(user2.child('tags'), childNode.child('title').val())){
	        cnt++;
	    }	
	});
	
	tagBasedWeight = (Number(cnt) / (Number(userTagCount1) + Number(userTagCount2) - Number(cnt)  )) / Number(userTagNecessityForMax);
	if(isNaN(tagBasedWeight)){
	    tagBasedWeight = Number(0);
	}
	tagBasedWeight = clamp(tagBasedWeight, 0, 1);

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
    var userTagCount = user.child('tagcount').exists() ? user.child('tagcount').val() : 0; console.log("USER TAGS:" + userTagCount);
    var postTagCount = post.child('tagcount').exists() ? post.child('tagcount').val() : 0; console.log("POST TAGS:" + postTagCount);
	user.child('tags').forEach(function(childNode){
	    if(doesTagExist(post.child('tags'), childNode.child('title').val())){
			cnt++;
		}	
	});
	var ret = Number(cnt)/(Number(postTagCount)*tagNecessityForMax);
	if( isNaN(ret) ) {
	    return Number(0);
	}
	return  Number(clamp(ret, 0, 1));
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

    console.log('coeft for user ' + post.key+ ' for the post ' + post.numChildren());
    value.child('Users').forEach(function(userChildNode){

            var coeft = cst(userChildNode, post);
            var coefu = csu(userChildNode, post);
            var x = baseref.child('Users').child(userChildNode.key).child('post_coef_list').child(post.key).set(coeft * tagCoefficientWeight + coefu * usersCoefficientWeight);
            console.log('for user ' + userChildNode.key + ' the coef was ' + coeft + tagCoefficientWeight + coefu + usersCoefficientWeight);
            console.log('THIS SHOULD HAPPEN SECOND');
    });
}



//Someone got added to the going or interested list of this post
exports.updateWeightsOnListAddTrigger = functions.database.ref('/{category}/{id}/{collection}/{changedelement}').onWrite(
    async(change)=> {
        console.log('WATITING FOR A BIT');

        if(autofill) wait(1000);//make sure counters are updated first
        console.log('WATITED FOR A BIT');

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
		        updatePostCoefsAll(baseref, value);

			    return;
		    });
	    }
    }
    return null;
});


exports.updateWeightsOnIdAddTrigger = functions.database.ref('/{category}/{id}').onWrite(
    async(change)=> {
        console.log('WATITING FOR A BIT');
        wait(1000);//make sure counters are updated first
        console.log('WATITED FOR A BIT');
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
        if (change.after.exists() && !change.before.exists()) {
            console.log('POST ADDED: ' + change.after.key);
            return baseref.once('value').then(function(value){

                updatePostCoefsPost(baseref, value.child('posts').child(post.key), value);

                return;
            });
        } else if (!change.after.exists() && change.before.exists()) {
            console.log('POST REMOVED: ' + change.before.key);
            return baseref.once('value').then(function(value){
                value.child('Users').forEach(function(userChildNode){
                    baseref.child('Users').child(userChildNode.key).child('post_coef_list').child(change.before.key).remove();
                    baseref.child('Users').child(userChildNode.key).child('interested_posts_id_list').child(change.before.key).remove();
                    baseref.child('Users').child(userChildNode.key).child('going_posts_id_list').child(change.before.key).remove();

                });
                return;
            });
        } else {
            return null;
        }
    }
    return null;
});


exports.validate = functions.database.ref('/{pending_posts}/{post}/{valid}').onWrite(
    async(change)=> {
        wait(1000);
        const baseref = change.after.ref.root;
       
        if(change.after.ref.parent.parent.key === 'pending_posts'){
                var postRef = change.after.ref.parent;
                if(change.after.key === 'valid' && change.after.exists()){
                    return postRef.once('value')
                        .then(function(post){
                            console.log('SEE IF THIS IS THE ISSUE        ' + post.child('tagcount').val());
                            var x = baseref.child('posts').child(post.key).set(post.val());
                            var y = baseref.child('pending_posts').child(post.key).remove();
                            return null;
                        });
                }
        }
        return null;
    });