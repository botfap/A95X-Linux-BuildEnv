
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <errno.h>
#include <amconfigutils.h>
#include <libavformat/ptslist.h>
int am_config_test()
{
    char value[32];
    int testresult;
    int kkk;
    float ff;
    printf("start am_config_test test \n");


#define testset(path,val)   \
    testresult=am_setconfig(path,val);\
    printf("set %s=%s, ret=%d\n",path,val,testresult);

    testset("hello.aboutyou", "yes");
    testset("hello.aboutyou2", "no");
    testset("buffer.level", "20000000");
    testset("buffer.level222", "600");
    testset("buffer.level222.123123333333333333333333333", "8000");
    testset("hello.666", "yes");
    testset("hello.555", "nnnnnnnn");
    testset("buffer.333", "pppppppp");
    testset("buffer.666", "ooooooo");
    testset("hello.kkkh", "yes");
    testset("hello.mmm", "nnnnnnnn");
    testset("buffer.rrrrrrrr", "pppppppp");
    testset("buffer.777777", "ooooooo");

#define printtestset(path,val)\
    value[0]='\0';\
    testresult =am_getconfig(path,value,"nosetting");\
    printf("get %s=%s,result=%d\n",path,value,testresult);

    printtestset("hello.aboutyou", "yes");
    printtestset("hello.aboutyou2", "no");
    printtestset("buffer.level", "20000000");
    printtestset("buffer.level222", "600");
    printtestset("buffer.level222.123123333333333333333333333", "8000");
    printtestset("hello.232", "yes");
    printtestset("hello.aboutyou244", "no");
    printtestset("buffer.level55", "20000000");
    printtestset("buffer.level22211", "600");


#define testsetint(path,val)    \
    testresult=am_setconfig_float(path,(float)val);\
    printf("set %s=%f, ret=%d\n",path,(float)val,testresult);


    testsetint("hello.aboutyou", -10000);
    testsetint("hello.aboutyou2", 0);
    testsetint("buffer.level", 888);
    testsetint("buffer.level22", 80999.999);
    testsetint("buffer.level", 666);
    testsetint("buffer.5555", 0.0000001);
    testsetint("buffer.56565", 0.00091119);

    printf("finished am_setconfig_int test \n");

#define printtestsetint(path,val)\
    ff=-1000;\
    testresult =am_getconfig_float(path,&ff);\
    printf("get %s=%f,result=%d\n",path,ff,testresult);

    printtestsetint("hello.aboutyou", -10000);
    printtestsetint("hello.aboutyou2", 0);
    printtestsetint("buffer.level", 12312312321);
    printtestsetint("buffer.level22", 12312312321);
    printtestsetint("buffer.5555", 0.0000001);
    printtestsetint("buffer.56565", 0.00091119);

    printf("finished printtestsetint test \n");
    testset("hello.mmm", "");
    testset("buffer.rrrrrrrr", "");
    am_dumpallconfigs();
    printf("finished am_config_test test \n");

    return 0;
}

int am_mode_test()
{
	int ret;
	struct ammodule_t *module;
#define testmodule(name)\
	ret=ammodule_load_module(name,&module);\
	if(ret==0){\
		ammodule_open_module(module);\
		printf("testmodule %s ok\n",name);\
	}
	
	testmodule("testmodule");
	testmodule("testmodule.so");
	testmodule("libtestmodule.so");
	testmodule("/system/lib/libtestmodule.so");
	testmodule("/system/lib/amplayer/libtestmodule");
	return 0;
}
int am_pts_test()
{
     ptslist_mgr_t *mgr;
     int a=1;
	 int b=2;
     int64_t pts;
    int margin;
    int ret;	 
     mgr=ptslist_alloc(0);
     ptslist_chekin(mgr,0,1000);
	 ptslist_chekin(mgr,200,1000);
	 ptslist_chekin(mgr,344,2000);
	 ptslist_chekin(mgr,800,3000);
	 ptslist_chekin(mgr,1026,4000);
	  ptslist_chekin(mgr,1226,5000);
	   ptslist_chekin(mgr,1426,6000);
	    ptslist_chekin(mgr,1826,9000);	
	 ptslist_chekin(mgr,1826,9000);
	 ptslist_chekin(mgr,2200,9600);
	 ptslist_chekin(mgr,2300,9800);
	 ptslist_chekin(mgr,2426,9900);
	 ptslist_chekin(mgr,2526,11000);
	 ptslist_chekin(mgr,2626,12000);
				
		 ptslist_dump_all(mgr);	
 
#define LOOKTEST(off,mr)	 \
     margin=mr;	\
     ret=ptslist_lookup(mgr,off,&pts,&margin);\
     printf("test look off:%lld ret=%d,pts=%lld,margin=%d,%d,%d....%x\n",(int64_t)off,ret,pts,margin,a,b,mgr);\


     LOOKTEST(100,200);
	 LOOKTEST(300,200);
	 LOOKTEST(400,200);
	 LOOKTEST(400,800);
	  LOOKTEST(400,-1);
	  LOOKTEST(800,0);
	  LOOKTEST(799,0);
	 LOOKTEST(500,200);
	 LOOKTEST(1800,200);

     ptslist_free(mgr);	
	 return 0;
}
int main(int argc, char **argv)
{

    printf("libplayer test start\n");
   // am_config_test();
    am_pts_test();
    printf("libplayer test end\n\n");
    return 0;
}
