#ifndef __WIDEVINE_SECURE_API_H__
#define __WIDEVINE_SECURE_API_H__

#ifdef __cplusplus
extern "C" {
#endif
	//allocate secure mem for es clear data after decrypting
	//length --memlen to be allocated, max 1M
    unsigned int Secure_AllocSecureMem(unsigned int length);
	// close secure session and release resource 
	unsigned int Secure_ReleaseDecrypt(); 
	// get NAL HAEDER and put it to decoder for tvp seeking issue
	//srccsdaddr-phyaddr with NAL header,
	// csd_addr--user space for feeding decoder fro user space
	//csd_len ---HEADR LENGTH
	unsigned int Secure_GetCsdData(unsigned int* srccsdaddr, unsigned char* csd_addr, unsigned int *csd_len);
    //get ge2d gurranty , guranty will be cancled 100ms later
    unsigned int Get_Ge2d_Guaranty(void); //return 1 sucess; return 0  failed 
#ifdef __cplusplus
}
#endif

#endif
