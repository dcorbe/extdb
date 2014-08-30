This is not finished yet...

Did some basic tests out local build with logging code commented out & without intel tbb.
Was working fine

Stuff left todo

Switch Poco Logging back to Boost logging...  (was in version 13 code) Already started it...
Just need to finish off updating the Protocols Logging Code

Add Intel tbb, i.e  dynamic Link it & add the source file memory_allocator.cpp

Come up with some basic optimize compiler settings for redistributable code

Would have it done tonight.... but some work came up... will be done this weekend.


Known issues...
Since i will be changing back to Boost Logging
Means Debian Users won't be able to compile from source without updating thier boost library.
Since there will be a static build of extDB, this won't be a showstopper issue.

Pluses of static build, could look @ backporting Poco changes for missing MySQL features i.e 
Date Time DataType Support