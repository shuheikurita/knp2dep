//Kurita

#pragma once

#include <sys/types.h>
#include <dirent.h>
#include "stdutil.h"

vecstr getfilelist(std::string path) {
	vecstr ss;
	DIR* dp=opendir(path.c_str());
	if(dp!=NULL) {
		struct dirent* dent;
		do {
			dent = readdir(dp);
			if(dent!=NULL)	ss.push_back(std::string(dent->d_name));
		} while(dent!=NULL);
		closedir(dp);
	}
	return ss;
}

