#include "libmigfpr_2.h"

#define HITFOR   2
#define NUMROT   24
#define MRESIZED 19
#define MTRTHR   5
*/

#define NSIGMA_WHITE 3

#define UI8UI16RATIO  0.0038910506f  /* ratio between maxuint8(255) and maxuint16(65535) */

******************************************************************************
*                               PRIVATE DATA
******************************************************************************
*/

{
    /* thread id */
    int id;

    mig_lst_t *Input;


{    
    /* svm model */
    mig_svm_t model;

    mig_svm_scale_t scales;

    int num_rot;

    int resized_len;

    int min_rot_pos_l2;
    int min_rot_pos_l3;
    int min_rot_pos_l4;
    int min_rot_pos_l5;
    int min_rot_pos_l6;
    int min_rot_pos_l7;
    int min_rot_pos_l8;
    int min_rot_pos_l9;
    int min_rot_pos_l10;
    int min_rot_pos_l11;
    int min_rot_pos_l12;
    int min_rot_pos_l13;
    int min_rot_pos_l14;
    int min_rot_pos_l15;

	/* 2d views thresholds */
    int min_pos_views_l2;
    int min_pos_views_l3;
    int min_pos_views_l4;
    int min_pos_views_l5;
    int min_pos_views_l6;
    int min_pos_views_l7;
    int min_pos_views_l8;
    int min_pos_views_l9;
    int min_pos_views_l10;
    int min_pos_views_l11;
    int min_pos_views_l12;
    int min_pos_views_l13;
    int min_pos_views_l14;
    int min_pos_views_l15;

    char *dir_results;
	
    /* threading */
    pthread_t           thread[2];      /* frp1 threads for left and right lung */ 
    fpr2_thread_data    thread_data[2]; /* fpr1 thread parameters for left and right lungs */


/*
******************************************************************************
*                               PRIVATE DATA
******************************************************************************
*/

static Logger _log =
	Logger::getInstance ( CAD_LOGGER_NAME );

static mig_cad_data_t *_CadData = NULL;

static fpr2_params_t _Fpr2Params;

******************************************************************************
*                               PRIVATE PROTOTYPE DECLARATIONS
******************************************************************************
*/

_fpr2_thread_routine ( void *arg );

_classify_obj_3d ( mig_im_region_t *obj3d , int *label );

_classify_obj_2d ( mig_im_region_t *obj2d , int *label , int objlen );

_get_hitfor_for_len ( int len );

_get_mrthr_for_len ( int len );


/*
******************************************************************************
*                               GLOBAL FUNCTIONS
******************************************************************************
*/

#if defined(WIN32)

DllMain ( HANDLE hModule , DWORD dwReason , LPVOID lpReserved )
{
	return TRUE;
}


******************************************************************************
*                               DLL INITIALIZATION
******************************************************************************
*/

mig_init ( mig_dic_t *d ,
           mig_cad_data_t *data )
{
    char *model_file_name = NULL;
    char *scale_file_name = NULL;
    int rc;

    /* setup logging system */
    char *_logger_ini_f_name =
        mig_ut_ini_getstring ( d , PARAM_LOG_INI , DEFAULT_PARAM_LOG_INI );
    if ( _logger_ini_f_name != NULL )
    {
        try
        {
            PropertyConfigurator::doConfigure ( _logger_ini_f_name );
        }
        catch ( ... )
        {
            fprintf ( stderr , "\nError loading log parameters from : %s..." , _logger_ini_f_name );
        }
    }

    LOG4CPLUS_DEBUG ( _log , " libmigfpr_2 -> mig_init starting..." );

    _CadData = data;

	/* load parameters from ini file */
    
	/* number of rotations */
	_Fpr2Params.num_rot = 
		mig_ut_ini_getint ( d , PARAM_FPR2_ROTATIONS , DEFAULT_PARAM_FPR2_ROTATIONS );
	_Fpr2Params.resized_len = 
		mig_ut_ini_getint ( d , PARAM_FPR2_RESIZED_LEN , DEFAULT_PARAM_FPR2_RESIZED_LEN );

	/* minimimum positive rotations */
	_Fpr2Params.min_rot_pos_l2 = 
        mig_ut_ini_getint ( d , PARAM_FPR2_MIN_ROT_POS_L2 , 0.0f );
	_Fpr2Params.min_rot_pos_l3 = 
        mig_ut_ini_getint ( d , PARAM_FPR2_MIN_ROT_POS_L3 , 0.0f );
	_Fpr2Params.min_rot_pos_l4 = 
        mig_ut_ini_getint ( d , PARAM_FPR2_MIN_ROT_POS_L4 , 0.0f );
	_Fpr2Params.min_rot_pos_l5 = 
        mig_ut_ini_getint ( d , PARAM_FPR2_MIN_ROT_POS_L5 , 0.0f );
	_Fpr2Params.min_rot_pos_l6 = 
        mig_ut_ini_getint ( d , PARAM_FPR2_MIN_ROT_POS_L6 , 0.0f );
	_Fpr2Params.min_rot_pos_l7 = 
        mig_ut_ini_getint ( d , PARAM_FPR2_MIN_ROT_POS_L7 , 0.0f );
	_Fpr2Params.min_rot_pos_l8 = 
        mig_ut_ini_getint ( d , PARAM_FPR2_MIN_ROT_POS_L8 , 0.0f );
	_Fpr2Params.min_rot_pos_l9 = 
        mig_ut_ini_getint ( d , PARAM_FPR2_MIN_ROT_POS_L9 , 0.0f );
	_Fpr2Params.min_rot_pos_l10 = 
        mig_ut_ini_getint ( d , PARAM_FPR2_MIN_ROT_POS_L10 , 0.0f );
	_Fpr2Params.min_rot_pos_l11 = 
        mig_ut_ini_getint ( d , PARAM_FPR2_MIN_ROT_POS_L11 , 0.0f );
	_Fpr2Params.min_rot_pos_l12 = 
        mig_ut_ini_getint ( d , PARAM_FPR2_MIN_ROT_POS_L12 , 0.0f );
	_Fpr2Params.min_rot_pos_l13 = 
        mig_ut_ini_getint ( d , PARAM_FPR2_MIN_ROT_POS_L13 , 0.0f );
	_Fpr2Params.min_rot_pos_l14 = 
        mig_ut_ini_getint ( d , PARAM_FPR2_MIN_ROT_POS_L14 , 0.0f );
	_Fpr2Params.min_rot_pos_l15 = 
        mig_ut_ini_getint ( d , PARAM_FPR2_MIN_ROT_POS_L15 , 0.0f );

	/* 3d eurystics minimum positive views */
	_Fpr2Params.min_pos_views_l2 = 
        mig_ut_ini_getint ( d , PARAM_FPR2_MIN_POS_VIEWS_L2 , 0.0f );
	_Fpr2Params.min_pos_views_l3 = 
        mig_ut_ini_getint ( d , PARAM_FPR2_MIN_POS_VIEWS_L3 , 0.0f );
	_Fpr2Params.min_pos_views_l4 = 
        mig_ut_ini_getint ( d , PARAM_FPR2_MIN_POS_VIEWS_L4 , 0.0f );
	_Fpr2Params.min_pos_views_l5 = 
        mig_ut_ini_getint ( d , PARAM_FPR2_MIN_POS_VIEWS_L5 , 0.0f );
	_Fpr2Params.min_pos_views_l6 = 
        mig_ut_ini_getint ( d , PARAM_FPR2_MIN_POS_VIEWS_L6 , 0.0f );
	_Fpr2Params.min_pos_views_l7 = 
        mig_ut_ini_getint ( d , PARAM_FPR2_MIN_POS_VIEWS_L7 , 0.0f );
	_Fpr2Params.min_pos_views_l8 = 
        mig_ut_ini_getint ( d , PARAM_FPR2_MIN_POS_VIEWS_L8 , 0.0f );
	_Fpr2Params.min_pos_views_l9 = 
        mig_ut_ini_getint ( d , PARAM_FPR2_MIN_POS_VIEWS_L9 , 0.0f );
	_Fpr2Params.min_pos_views_l10 = 
        mig_ut_ini_getint ( d , PARAM_FPR2_MIN_POS_VIEWS_L10 , 0.0f );
	_Fpr2Params.min_pos_views_l11 = 
        mig_ut_ini_getint ( d , PARAM_FPR2_MIN_POS_VIEWS_L11 , 0.0f );
	_Fpr2Params.min_pos_views_l12 = 
        mig_ut_ini_getint ( d , PARAM_FPR2_MIN_POS_VIEWS_L12 , 0.0f );
	_Fpr2Params.min_pos_views_l13 = 
        mig_ut_ini_getint ( d , PARAM_FPR2_MIN_POS_VIEWS_L13 , 0.0f );
	_Fpr2Params.min_pos_views_l14 = 
        mig_ut_ini_getint ( d , PARAM_FPR2_MIN_POS_VIEWS_L14 , 0.0f );
	_Fpr2Params.min_pos_views_l15 = 
        mig_ut_ini_getint ( d , PARAM_FPR2_MIN_POS_VIEWS_L15 , 0.0f );

    /* get model file name from ini file */
    model_file_name = mig_ut_ini_getstring ( d , PARAM_FPR2_SVM_MODEL_FILE , NULL );
    if ( model_file_name == NULL )
    {
        LOG4CPLUS_FATAL ( _log , " Ini file does not contain a model file..." );
        return MIG_ERROR_PARAM;
    }

	/* get scale file name from ini file */
    scale_file_name = mig_ut_ini_getstring ( d , PARAM_FPR2_SVM_NORM_FILE , NULL );
    if ( scale_file_name == NULL )
    {
        LOG4CPLUS_FATAL ( _log , " Ini file does not contain a scale file..." );
        return MIG_ERROR_PARAM;
    }

    rc = mig_svm_scale_params_load ( scale_file_name , &( _Fpr2Params.scales ) );
    if ( rc != MIG_OK )
    {
        mig_svm_scale_params_free ( &( _Fpr2Params.scales ) );
        LOG4CPLUS_FATAL ( _log , " Could not load scales file : " << scale_file_name );
        return rc;
    }

    rc = mig_svm_model_load ( model_file_name , &( _Fpr2Params.model ) );
    if ( rc != MIG_OK )
    {
        mig_svm_model_free ( &( _Fpr2Params.model ) );
        LOG4CPLUS_FATAL ( _log , " Could not load model file : " << model_file_name );
        return rc;
    }

	/* results */
	_Fpr2Params.dir_results = 
		mig_ut_ini_getstring ( d , PARAM_CAD_DIR_OUT , DEFAULT_PARAM_CAD_DIR_OUT );

    if ( _log.getLogLevel() <= INFO_LOG_LEVEL )
    {
        std::stringstream os;
		os << "\nFPR2 parameters : ";
        os << "\nModel file  : " << model_file_name;
        os << "\nScales file : " << scale_file_name;
        LOG4CPLUS_INFO ( _log , os.str() );
    }

    LOG4CPLUS_DEBUG ( _log , " libmigfpr_2 -> mig_init done..." );
    return MIG_OK;
}

******************************************************************************
*                               DLL MAIN FUNCTION
******************************************************************************
*/

mig_run ()
{
    int rc, rc1, rc2;



	/******************************************************************************/
    /* try loading fpr2 point from disc */
  
    /* right lung */
    snprintf ( path , MAX_PATH , "%s%c%s_%s_%s_fpr2_r.txt" ,
                        _Fpr2Params.dir_results , MIG_PATH_SEPARATOR ,
                        _CadData->dicom_data.patient_id ,
                        _CadData->dicom_data.study_uid ,
                        _CadData->dicom_data.series_uid );

    rc1 = mig_tag_read ( path , &( _CadData->det_r ) );
    if ( rc1 == MIG_OK )
    {
        _CadData->det_r._free = &free;
        LOG4CPLUS_INFO ( _log , " Loaded right lung fpr2 data from disk...");
    }
    
    /* left lung */
    snprintf ( path , MAX_PATH , "%s%c%s_%s_%s_fpr2_l.txt" ,
               _Fpr2Params.dir_results , MIG_PATH_SEPARATOR ,
               _CadData->dicom_data.patient_id ,
               _CadData->dicom_data.study_uid ,
               _CadData->dicom_data.series_uid );

    rc2 = mig_tag_read ( path , &( _CadData->det_l ) );
    if ( rc2 == MIG_OK )
    {
        _CadData->det_l._free = &free;
        LOG4CPLUS_INFO ( _log , " Loaded left lung fpr2 data from disk...");
    }
    
    if ( ( rc1 == MIG_OK ) && ( rc2 == MIG_OK ) )
    {
		_CadData ->det_cleanup  = &free;
		_CadData ->fpr1_cleanup  = &free;
        LOG4CPLUS_INFO ( _log , " Using fpr2 data from disk...");
        return MIG_OK;
    }


    _Fpr2Params.thread_data[0].id      = 0;
    _Fpr2Params.thread_data[0].Input   = &( _CadData->det_r );

	_Fpr2Params.thread_data[1].id      = 1;
    _Fpr2Params.thread_data[1].Input   = &( _CadData->det_l );

    rc = pthread_create ( &( _Fpr2Params.thread[0] ) , NULL ,
            &( _fpr2_thread_routine ),  &( _Fpr2Params.thread_data[0] ) );

	if ( rc != 0 )
    {
        LOG4CPLUS_FATAL ( _log , " libmigfpr_2 -> mig_run pthread_create returned : " << rc );
        return MIG_ERROR_THREAD;
    }
   
    /* spawn detection thread 1 */
    rc = pthread_create ( &( _Fpr2Params.thread[1] ) , NULL ,
            &( _fpr2_thread_routine ),  &( _Fpr2Params.thread_data[1] ) );
    if ( rc != 0 )
    {
        LOG4CPLUS_FATAL ( _log , " libmigfpr_2 -> mig_run pthread_create returned : " << rc );
        return MIG_ERROR_THREAD;
    }

    pthread_join ( _Fpr2Params.thread[0] , (void**) &rc );
   
    /* wait for right thread to finish */
    pthread_join ( _Fpr2Params.thread[1] , (void**) &rc ); 
   
    LOG4CPLUS_DEBUG ( _log , " libmigfpr_2 -> mig_run end..." );

	/******************************************************************************/
    /* save fpr2 lists to disk */
   
    /* right lung */
    snprintf ( path , MAX_PATH , "%s%c%s_%s_%s_fpr2_r.txt" ,
                        _Fpr2Params.dir_results , MIG_PATH_SEPARATOR ,
                        _CadData->dicom_data.patient_id ,
                        _CadData->dicom_data.study_uid ,
                        _CadData->dicom_data.series_uid );

    rc = mig_tag_write ( path , &( _CadData->det_r ) );
    if ( rc != MIG_OK )
    {
        LOG4CPLUS_WARN( _log , " Could not save fpr2 results for right lung to disk..." );
    }
    else
    {
        LOG4CPLUS_INFO( _log , " Saved fpr2 results for right lung to disk..." );
    }

    snprintf ( path , MAX_PATH , "%s%c%s_%s_%s_fpr2_l.txt" ,
                        _Fpr2Params.dir_results , MIG_PATH_SEPARATOR ,
                        _CadData->dicom_data.patient_id ,
                        _CadData->dicom_data.study_uid ,
                        _CadData->dicom_data.series_uid );

    rc = mig_tag_write ( path , &( _CadData->det_l ) );
    if ( rc != MIG_OK )
    {
        LOG4CPLUS_WARN( _log , " Could not save fpr2 results for left lung to disk..." );
    }
    else
    {
        LOG4CPLUS_INFO( _log , " Saved fpr2 results for left lung to disk..." );
    }

    return MIG_OK;
}

******************************************************************************
*                               DLL CLEANUP
******************************************************************************
*/

mig_cleanup ( void* data )
{
    if ( data )
        free ( data );
}

******************************************************************************
*                               DLL INFORMATION
******************************************************************************
*/

mig_info ( mig_dll_info_t* info )
{

}

******************************************************************************
*                               PRIVATE PROTOTYPE DEFINITIONS
******************************************************************************
*/

_fpr2_thread_routine ( void *arg )
{
    mig_lst_t survived_objs = { NULL , NULL , 0 , free };      /* list of survived 3d objects */
    mig_im_region_t *curr;                                      /* current 3d region */
    fpr2_thread_data *data = (fpr2_thread_data*) arg;           /* input parameters */
    int label = 0;
    int rc;
    
    LOG4CPLUS_DEBUG ( _log , " _fpr2_thread_routine : " << data->id );

    {
        LOG4CPLUS_DEBUG ( _log , " _fpr2_thread_routine : " << data->id , " , Input data was empty..." );
        return ( (void*)MIG_OK );
    }

    while ( curr = (mig_im_region_t*) mig_lst_get_head ( data->Input ) )
    {
		/* classify 3d object */
        rc = _classify_obj_3d ( curr , &label );
        if ( rc != MIG_OK )
        {
            LOG4CPLUS_FATAL ( _log , "Aborting fpr2 thread. Error : " <<  rc << " in : " << data->id );
            pthread_exit ( (void*)rc );    
        }

        if ( label == 1 )
            mig_lst_put_tail ( &survived_objs , curr );
        /* if label is 0 free all 2d objects belonging to current 3d object */
        else
        {
			mig_lst_free_custom_static_data_and_node ( &( curr->objs ) , _CadData->det_cleanup , _CadData->fpr1_cleanup);
			_CadData->det_cleanup ( curr );
        }
            
    } /* while */
    
    /* copy survived list into input list */
    mig_lst_cat ( &survived_objs , data->Input );

}


_classify_obj_3d ( mig_im_region_t *obj3d , int *label )
{
    mig_lst_iter it;
    mig_im_region_t *obj2d;
    int rc;
    int pos = 0 , neg = 0;
    int len;
    int curr_label;
    int hitfor; 

    len = mig_lst_len ( &( obj3d->objs ) );
	
	hitfor = _get_hitfor_for_len ( len );

    mig_lst_iter_get ( &it , &( obj3d->objs ) );

    while ( obj2d = (mig_im_region_t*) mig_lst_iter_next ( &it ) )
    {
        /* if number of positive classifications is high enough
           consider current 3d object as positive */
        if ( pos >= hitfor )
        {
            *label = 1;
            break;
        }

           consider current 3d object as negative */
        if ( neg > ( len - hitfor ) )
        {
            *label = 0;
            break;
        }

        if ( rc != MIG_OK )
            return rc;
        
        if ( curr_label == 1 )
            pos ++ ;
        else
            neg ++;
    }

}


_classify_obj_2d ( mig_im_region_t *obj2d , int *label , int objlen )
{   
    int i;
    int diam_full , r_full;
    int diam_valid , r_valid;
    float *crop = NULL , *rot = NULL , *cut = NULL , *resized = NULL, *whitened = NULL;
    unsigned short *slice;
    int pos = 0 , neg = 0 , tot;
    float angle = 0.0f , angle_inc;
    float min , max;
    mig_svm_example_t x;


	tot = _Fpr2Params.num_rot;
    
    /* crop cutting radii */
    r_full  = (int) ( 2.0f * MIG_SQRT2 *( obj2d->radius * 2.0f + 0.5f ) );
    r_valid = (int) ( 2.0f *( obj2d->radius * 2.0f + 0.5f ) );

    diam_full = 2 * r_full + 1;
    diam_valid = 2 * r_valid + 1;

    crop = (float*) malloc ( MIG_POW2( diam_full ) * sizeof(float) );
    if ( crop == NULL )
        return MIG_ERROR_MEMORY;

    
    /* cut crop */
    mig_im_bb_cut_2d ( slice  , _CadData->stack_s.w , _CadData->stack_s.h ,
                       (int) ( obj2d->centroid[0] ) , (int) ( obj2d->centroid[1] ) , 
                       crop , r_full );

    //mig_im_util_min_max_32f ( crop , MIG_POW2( diam_full ) , &min , &max );

	//AAAAAAAAA This way whitening doesn't work
	//here we need to transform to float [0.,255.]
    //if ( min != max )
    //    mig_im_util_mat2gray_32f ( crop , MIG_POW2( diam_full ) , min , max );

    mig_im_alg_constmul (crop , MIG_POW2( diam_full ) , UI8UI16RATIO );

    angle_inc = 360.0f / tot;

    rot = (float*) calloc ( MIG_POW2( diam_full ) , sizeof(float) );
    if ( rot == NULL )
        goto error;
    
    /* valid part of rotated image */
    cut = (float*) calloc ( MIG_POW2( diam_valid ) , sizeof(float) );
    if ( cut == NULL )
        goto error;

	resized = (float*) calloc ( MIG_POW2( _Fpr2Params.resized_len ) , sizeof(float) );
    if ( resized == NULL )
        goto error;
	
    /* whitened image buffer */
	whitened = (float*) calloc ( MIG_POW2( _Fpr2Params.resized_len ) , sizeof(float) );
    if ( whitened == NULL )
        goto error;

    for ( i = 0 ; i < tot ; ++i , angle += angle_inc )
    {
        /* if number of positive classifications is high enough
           consider current 2d object as positive */
        if ( pos >= mrthr )
        {
            *label = 1;
            break;
        }

           consider current 2d object as negative */
        if ( neg > ( tot - mrthr ) )
        {
            *label = 0;
            break;
        }

        mig_im_geom_rotate ( crop , rot , diam_full , diam_full , angle , BILINEAR );

        mig_im_bb_cut_2d_32f ( rot , diam_full , diam_full , r_full , r_full , cut , r_valid );

        /*mig_im_geom_resize ( cut , diam_valid , diam_valid , resized , MRESIZED , MRESIZED , BICUBIC ); */
		mig_im_geom_resize ( cut , diam_valid , diam_valid , resized , _Fpr2Params.resized_len , _Fpr2Params.resized_len , BILINEAR );
        /* whitening */	
        mig_im_scale_whitening ( resized, whitened , NSIGMA_WHITE,
            _Fpr2Params.scales.len , _Fpr2Params.scales.mean ,  _Fpr2Params.scales.std );
        
        /* load data into svm suitable format */
        /* x.len  = MIG_POW2( diam_valid ); */
		x.len  = MIG_POW2( _Fpr2Params.resized_len );
	    x.feat = whitened;
    
        /* label prediction */
        if ( mig_svm_predict ( &( _Fpr2Params.model ) , &x ) != MIG_OK )
            goto error;

            pos ++ ;
        else
            neg ++;

        memset ( rot     , 0x00 ,  MIG_POW2( diam_full  ) * sizeof(float) );
        memset ( cut     , 0x00 ,  MIG_POW2( diam_valid ) * sizeof(float) );
		memset ( resized , 0x00 ,  MIG_POW2( _Fpr2Params.resized_len ) * sizeof(float) );
		memset ( whitened , 0x00 ,  MIG_POW2( _Fpr2Params.resized_len ) * sizeof(float) );
    }
    
    free ( crop );
    free ( rot );
    free ( cut );
    free ( resized );
	free ( whitened );



	if ( crop )
        free ( crop );

        free ( rot );

        free ( cut );

        free ( resized );

		free ( whitened );
    
    return MIG_ERROR_MEMORY;
}

static int
_get_hitfor_for_len ( int len ){
	switch (len){
		case 2:
			return _Fpr2Params.min_pos_views_l2;
			break;
		case 3:
			return _Fpr2Params.min_pos_views_l3;
			break;
		case 4:
			return _Fpr2Params.min_pos_views_l4;
			break;
		case 5:
			return _Fpr2Params.min_pos_views_l5;
			break;
		case 6:
			return _Fpr2Params.min_pos_views_l6;
			break;
		case 7:
			return _Fpr2Params.min_pos_views_l7;
			break;
		case 8:
			return _Fpr2Params.min_pos_views_l8;
			break;
		case 9:
			return _Fpr2Params.min_pos_views_l9;
			break;
		case 10:
			return _Fpr2Params.min_pos_views_l10;
			break;
		case 11:
			return _Fpr2Params.min_pos_views_l11;
			break;
		case 12:
			return _Fpr2Params.min_pos_views_l12;
			break;
		case 13:
			return _Fpr2Params.min_pos_views_l13;
			break;
		case 14:
			return _Fpr2Params.min_pos_views_l14;
			break;
		case 15:
			return _Fpr2Params.min_pos_views_l15;
			break;
		default:
			return MIG_ERROR_PARAM;
	}
}

static int
_get_mrthr_for_len ( int len ){
	switch (len){
		case 2:
			return _Fpr2Params.min_rot_pos_l2;
			break;
		case 3:
			return _Fpr2Params.min_rot_pos_l3;
			break;
		case 4:
			return _Fpr2Params.min_rot_pos_l4;
			break;
		case 5:
			return _Fpr2Params.min_rot_pos_l5;
			break;
		case 6:
			return _Fpr2Params.min_rot_pos_l6;
			break;
		case 7:
			return _Fpr2Params.min_rot_pos_l7;
			break;
		case 8:
			return _Fpr2Params.min_rot_pos_l8;
			break;
		case 9:
			return _Fpr2Params.min_rot_pos_l9;
			break;
		case 10:
			return _Fpr2Params.min_rot_pos_l10;
			break;
		case 11:
			return _Fpr2Params.min_rot_pos_l11;
			break;
		case 12:
			return _Fpr2Params.min_rot_pos_l12;
			break;
		case 13:
			return _Fpr2Params.min_rot_pos_l13;
			break;
		case 14:
			return _Fpr2Params.min_rot_pos_l14;
			break;
		case 15:
			return _Fpr2Params.min_rot_pos_l15;
			break;
		default:
			return MIG_ERROR_PARAM;
	}
}