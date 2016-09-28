#include<ammodule.h>

#include <android/log.h>
#define  LOG_TAG    "testmodule"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)


ammodule_methods_t  amtest_module_methods;
ammodule_t AMPLAYER_MODULE_INFO_SYM = {
        tag: AMPLAYER_MODULE_TAG,
        version_major: AMPLAYER_API_MAIOR,
        version_minor: AMPLAYER_API_MINOR,
        id: 0,
        name: "amtestmodule",
        author: "Amlogic",
        descript:"ammodule for test and example..",
        methods: &amtest_module_methods,
        dso : NULL,
        reserved : {0},
};
	
int testinit(const struct ammodule_t* module, int flags)
{
      LOGI("--testinit--------");
      return 0;
}

int testrelease(const struct ammodule_t* module)
{
	LOGI("--testrelease--------");
	return 0;
}


ammodule_methods_t  amtest_module_methods={
   .init =	testinit,
   .release =	testrelease,
} ;

