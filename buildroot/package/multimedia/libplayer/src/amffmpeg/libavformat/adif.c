
#include "adif.h"


/**************************************************************************************
 * Function:    DecodeProgramConfigElement
 *
 * Description: decode one PCE
 *
 * Inputs:      BitStreamInfo struct pointing to start of PCE (14496-3, table 4.4.2) 
 *
 * Outputs:     filled-in ProgConfigElement struct
 *              updated BitStreamInfo struct
 *
 * Return:      0 if successful, error code (< 0) if error
 *
 * Notes:       #define KEEP_PCE_COMMENTS to save the comment field of the PCE
 *                (otherwise we just skip it in the bitstream, to save memory)
 **************************************************************************************/
static int DecodeProgramConfigElement(ProgConfigElement *pce, GetBitContext *bsi)
{
	int i;

	pce->elemInstTag =   get_bits(bsi, 4);
	pce->profile =       get_bits(bsi, 2);
	pce->sampRateIdx =   get_bits(bsi, 4);
	pce->numFCE =        get_bits(bsi, 4);
	pce->numSCE =        get_bits(bsi, 4);
	pce->numBCE =        get_bits(bsi, 4);
	pce->numLCE =        get_bits(bsi, 2);
	pce->numADE =        get_bits(bsi, 3);
	pce->numCCE =        get_bits(bsi, 4);

	pce->monoMixdown = get_bits(bsi, 1) << 4;	/* present flag */
	if (pce->monoMixdown)
		pce->monoMixdown |= get_bits(bsi, 4);	/* element number */

	pce->stereoMixdown = get_bits(bsi, 1) << 4;	/* present flag */
	if (pce->stereoMixdown)
		pce->stereoMixdown  |= get_bits(bsi, 4);	/* element number */

	pce->matrixMixdown = get_bits(bsi, 1) << 4;	/* present flag */
	if (pce->matrixMixdown) {
		pce->matrixMixdown  |= get_bits(bsi, 2) << 1;	/* index */
		pce->matrixMixdown  |= get_bits(bsi, 1);			/* pseudo-surround enable */
	}

	for (i = 0; i < pce->numFCE; i++) {
		pce->fce[i]  = get_bits(bsi, 1) << 4;	/* is_cpe flag */
		pce->fce[i] |= get_bits(bsi, 4);			/* tag select */
	}

	for (i = 0; i < pce->numSCE; i++) {
		pce->sce[i]  = get_bits(bsi, 1) << 4;	/* is_cpe flag */
		pce->sce[i] |= get_bits(bsi, 4);			/* tag select */
	}

	for (i = 0; i < pce->numBCE; i++) {
		pce->bce[i]  = get_bits(bsi, 1) << 4;	/* is_cpe flag */
		pce->bce[i] |= get_bits(bsi, 4);			/* tag select */
	}

	for (i = 0; i < pce->numLCE; i++)
		pce->lce[i] = get_bits(bsi, 4);			/* tag select */

	for (i = 0; i < pce->numADE; i++)
		pce->ade[i] = get_bits(bsi, 4);			/* tag select */

	for (i = 0; i < pce->numCCE; i++) {
		pce->cce[i]  = get_bits(bsi, 1) << 4;	/* independent/dependent flag */
		pce->cce[i] |= get_bits(bsi, 4);			/* tag select */
	}


	align_get_bits(bsi);

#if 0//def KEEP_PCE_COMMENTS
	pce->commentBytes = get_bits(bsi, 8);
	for (i = 0; i < pce->commentBytes; i++)
		pce->commentField[i] = get_bits(bsi, 8);
#else
	/* eat comment bytes and throw away */
	i = get_bits(bsi, 8);
	while (i--)
		get_bits(bsi, 8);
#endif

	return 0;
}
/**************************************************************************************
 * Function:    GetNumChannelsADIF
 *
 * Description: get number of channels from program config elements in an ADIF file
 *
 * Inputs:      array of filled-in program config element structures
 *              number of PCE's
 *
 * Outputs:     none
 *
 * Return:      total number of channels in file
 *              -1 if error (invalid number of PCE's or unsupported mode)
 **************************************************************************************/
static int GetNumChannelsADIF(ProgConfigElement *fhPCE, int nPCE)
{
	int i, j, nChans;

	if (/*nPCE < 1 ||*/ nPCE > MAX_NUM_PCE_ADIF)
		return -1;

	nChans = 0;
	for (i = 0; i < nPCE; i++) {
		av_log(NULL, AV_LOG_INFO,"ADIF_INFO:fhPCE[i].profile=%d fhPCE[i].numCCE=%d\n",fhPCE[i].profile,fhPCE[i].numCCE);
        #if 0
		/* for now: only support LC, no channel coupling */
		if (fhPCE[i].profile != 1/*LC*/ || fhPCE[i].numCCE > 0)
			return -1;
        #endif
		/* add up number of channels in all channel elements (assume all single-channel) */
        nChans += fhPCE[i].numFCE;
        nChans += fhPCE[i].numSCE;
        nChans += fhPCE[i].numBCE;
        nChans += fhPCE[i].numLCE;

		/* add one more for every element which is a channel pair */
        for (j = 0; j < fhPCE[i].numFCE; j++) {
            if (CHAN_ELEM_IS_CPE(fhPCE[i].fce[j]))
                nChans++;
        }
        for (j = 0; j < fhPCE[i].numSCE; j++) {
            if (CHAN_ELEM_IS_CPE(fhPCE[i].sce[j]))
                nChans++;
        }
        for (j = 0; j < fhPCE[i].numBCE; j++) {
            if (CHAN_ELEM_IS_CPE(fhPCE[i].bce[j]))
                nChans++;
        }

	}

	return nChans;
}
/**************************************************************************************
 * Function:    GetSampleRateIdxADIF
 *
 * Description: get sampling rate index from program config elements in an ADIF file
 *
 * Inputs:      array of filled-in program config element structures
 *              number of PCE's
 *
 * Outputs:     none
 *
 * Return:      sample rate of file
 *              -1 if error (invalid number of PCE's or sample rate mismatch)
 **************************************************************************************/
 static int GetSampleRateIdxADIF(ProgConfigElement *fhPCE, int nPCE)
{
	int i, idx;

	if (nPCE < 1 || nPCE > MAX_NUM_PCE_ADIF)
		return -1;

	/* make sure all PCE's have the same sample rate */
	idx = fhPCE[0].sampRateIdx;
	for (i = 1; i < nPCE; i++) {
		if (fhPCE[i].sampRateIdx != idx)
			return -1;
	}

	return idx;
}

int adif_header_parse(AVStream *st,ByteIOContext *pb)
{ 
	GetBitContext gbc;
	ADIFHeader hADIF;	
	ADIFHeader *fhADIF = &hADIF;	
	int ch,sr_index,i;
	ProgConfigElement pce[MAX_NUM_PCE_ADIF];
	const int aac_sample_rates[16] = {
    	96000, 88200, 64000, 48000, 44100, 32000,
    	24000, 22050, 16000, 12000, 11025, 8000, 7350
	};
	init_get_bits(&gbc, pb->buffer+4, pb->buffer_size-4);//skip adif tag
	/* read ADIF header fields */
	fhADIF->copyBit = get_bits(&gbc, 1);
	if (fhADIF->copyBit) {
	for (i = 0; i < ADIF_COPYID_SIZE; i++)
		fhADIF->copyID[i] = get_bits(&gbc,8);
	}
	fhADIF->origCopy = get_bits(&gbc, 1);
	fhADIF->home =     get_bits(&gbc, 1);
	fhADIF->bsType =   get_bits(&gbc,1);
	fhADIF->bitRate =  get_bits(&gbc,23);
	fhADIF->numPCE =   get_bits(&gbc, 4) + 1;	/* add 1 (so range = [1, 16]) */
	/* parse all program config elements */
	for (i = 0; i < fhADIF->numPCE; i++){
	   if (fhADIF->bsType == 0)
		   fhADIF->bufferFull = get_bits(&gbc, 20);
	   else
	   	   fhADIF->bufferFull = 0;
		DecodeProgramConfigElement(pce + i, &gbc);
	}

	/* byte align */
	align_get_bits(&gbc);

	/* update codec info */
	ch = GetNumChannelsADIF(pce, 1/*fhADIF->numPCE*/);
	sr_index = GetSampleRateIdxADIF(pce, fhADIF->numPCE);

	av_log(st, AV_LOG_INFO,"ADIF_INFO:ch=%d sr_index=%d\n",ch,sr_index);
	//	/* check validity of header */
	if (ch < 0 || sr_index < 0 || sr_index >= NUM_SAMPLE_RATES){
		
		return -1;
	}	
	st->codec->bit_rate = 	fhADIF->bitRate;
	st->codec->channels = ch;
	st->codec->sample_rate = aac_sample_rates[sr_index];
	st->codec->profile = pce[0].profile;
	av_log(st, AV_LOG_INFO," sr %d,ch %d,bitraete %d,profile %d",st->codec->sample_rate ,st->codec->channels,\
		st->codec->bit_rate,st->codec->profile);
	return 0;
}

