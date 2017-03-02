#include "libmigfpr_1.h"

/*
******************************************************************************
*                               PRIVATE DATA
******************************************************************************
*/

/*
******************************************************************************
* FPR1 THREAD INPUT STRUCTURE
******************************************************************************
*/

typedef struct _fpr1_thread_data
{
    /* thread id */
    int id;

    /* Input data -> results of detection */
    mig_lst_t *Input;   /* list of mig_im_region_t structures */

} fpr1_thread_data;

/*
******************************************************************************
* FPR1 PARAMETERS ACCESSIBLE BY ALL THREADS
******************************************************************************
*/

typedef struct _fpr1_params_t
{    
    /* parameters for building the 3d regions */
    float delta_tol;        /* maximum distance in pixels in x,y plane for point inclusion */
    float max_scan_dist;    /* maximum distance in slices in z plane for point inclusion */

    /* parameters for cutting 3d regions */
    int max_obj_length;             /* maximum allowed object lenght in z in pixels */

    int apply_only_radius;
    int skip_build3d;

	float min_max_radius;
	float max_max_radius;
    
    float min_mean_GL;
	float max_mean_GL;

    float max_angle_l2; /* maximum inclination angle allowed for structures of z lenght 2 */
    float max_angle_l3; /* maximum inclination angle allowed for structures of z lenght 3 */ 
    float max_angle_l4;
    float max_angle_l5;
    float max_angle_l6;
    float max_angle_l7;
    float max_angle_l8;
    float max_angle_l9;
    float max_angle_l10;

    float max_obj_volume_low_l2;    /* maximum volume in pixel allowed for structures of z lenght 2 low threshold */
    float max_obj_volume_low_l3;
    float max_obj_volume_low_l4;
    float max_obj_volume_low_l5;
    float max_obj_volume_low_l6;
    float max_obj_volume_low_l7;
    float max_obj_volume_low_l8;
    float max_obj_volume_low_l9;
    float max_obj_volume_low_l10;

    float max_obj_volume_high_l2; /* maximum volume in pixel allowed for structures of z lenght 2 hight threshold */
    float max_obj_volume_high_l3;
    float max_obj_volume_high_l4;
    float max_obj_volume_high_l5;
    float max_obj_volume_high_l6;
    float max_obj_volume_high_l7;
    float max_obj_volume_high_l8;
    float max_obj_volume_high_l9;
    float max_obj_volume_high_l10;

	/* results directory where to dump intermediate results */
    char *dir_results;

    /* threading */
    pthread_t           thread[2];      /* frp1 threads for left and right lung */ 
    fpr1_thread_data    thread_data[2]; /* fpr1 thread parameters for left and right lungs */

} fpr1_params_t;

/*
******************************************************************************
*                               PRIVATE DATA
******************************************************************************
*/

/* logger for the whole cad */
static Logger _log =
	Logger::getInstance ( CAD_LOGGER_NAME );

/* pointer to global data structure */
static mig_cad_data_t *_CadData = NULL;

/* fpr1 parameters */
static fpr1_params_t _Fpr1Params;

/*
******************************************************************************
*                               PRIVATE PROTOTYPE DECLARATIONS
******************************************************************************
*/

/*
******************************************************************************
*                       COMPARE TWO 3D OBJECTS
*
* Description : This function compares two 3D objects for similarity. The following
*               criteria are used :
*                       1. Distance along z axis : should be inferior to max_scan_dist
*                       2. Distance in x,y plane : should be inferior to delta_tol
*
* Arguments   :  a,b - 3d objects to compare ( of type mig_im_region_t )
*
* Returns     :  0  if objects satisfy distance criteria
*                -1 if objects do not satisfy distance criteria
*
******************************************************************************
*/

static int 
_build_obj3d_compare ( const void *a , const void *b );

/*
******************************************************************************
*                       APPLY THRESHOLDS ON 3D OBJECTS
*
* Description : This function applys cutting on a 3d object using thresholds contained in
*               a _fpr1_params_t structure.
*
* Arguments   :  a - 3d objects to test ( of type mig_im_region_t )
*
* Returns     :  0 if object is to be eliminated
*               -1 if objects is to be kept
*
******************************************************************************
*/

static int 
_build_obj3d_select  ( const void *a );

/*
******************************************************************************
*                       CALCULATE VOLUME OF 3D OBJECTS
*
* Description : This function calculates the volume in pixels of a 3d object.
*
* Arguments   :  reg - 3d object.
*
* Returns     :  volume in pixels.
*
* Notes       : Volume is calcualted as sum of circular areas of each 2d object
*               makeing up the current 3d object.
*
******************************************************************************
*/

static float
_obj3d_volume ( mig_im_region_t *reg );

/*
******************************************************************************
*                       CALCULATE SLANT ANGLE OF 3D OBJECTS
*
* Description : This function calculates the slant angle of a 3d object.
*
* Arguments   :  reg - 3d object.
*
* Returns     :  angle in degrees
*
* Notes       : Angle is calculated using linear fitting on the centroids of all 2d objects
*               makeing up the current 3d object.
*
******************************************************************************
*/

static float
_obj3d_angle ( mig_im_region_t *reg );


/*
******************************************************************************
*               CALCULATE GREATEST RADIUS OF 2D SLICES OF 3D OBJECTS
*
* Description : This function calculates the maximum radius among 2D slices of 3d object.
*
* Arguments   :  reg - 3d object.
*
* Returns     :  radius in float
*
* Notes       : 
*
******************************************************************************
*/

static float
_obj3d_max_radius ( mig_im_region_t *reg );

/******************************************************************************
*               CALCULATE MEAN GRAY LEVEL OF 2D CROPS OF 3D OBJECTS
*
* Description : This function calculates the maximum radius among 2D slices of 3d object.
*
* Arguments   :  reg - 3d object.
*
* Returns     :  mean gray level in float
*
* Notes       : 
*
******************************************************************************
*/

static float
_obj3d_mean_GL ( mig_im_region_t *reg );

/******************************************************************************
*               CALCULATE MEAN GRAY LEVEL OF 2D CROP
*
* Description : This function calculates the maximum radius among 2D slices of 3d object.
*
* Arguments   :  reg - 2d object.
*
* Returns     :  mean gray level in float
*
* Notes       : 
*
******************************************************************************
*/

static float
_obj2d_mean_GL ( mig_im_region_t *reg );


/*
******************************************************************************
*                       FPR1 THREAD ROUTINE
*
* Description : Thread routine for FPR1 on single segmented stack.
*
* Arguments   :  arg - input of type _fpr1_thread_data.
*
******************************************************************************
*/

static void*
_fpr1_thread_routine ( void *arg );

/*
******************************************************************************
*                               GLOBAL FUNCTIONS
******************************************************************************
*/

/* DLL entry for Windows */
#if defined(WIN32)

BOOL APIENTRY
DllMain ( HANDLE hModule ,
          DWORD dwReason ,
          LPVOID lpReserved )
{
	return TRUE;
}

#endif /* Win32 DLL */

/*
******************************************************************************
*                               DLL INITIALIZATION
******************************************************************************
*/

int
mig_init ( mig_dic_t *d ,
           mig_cad_data_t *data )
{
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
            fprintf ( stderr ,
                    "\nError loading log parameters from : %s..." ,
                    _logger_ini_f_name );
        }
    }

    /* here we've got a logging system so log what we are doing */
    LOG4CPLUS_DEBUG ( _log , " libmigfpr_1 -> mig_init starting..." );

    /* global cad data */
    _CadData = data;

    /* load parameters from ini file */
    
    /* 3d object building parameters */
    
    _Fpr1Params.delta_tol = 
        mig_ut_ini_getfloat ( d , PARAM_FPR1_DELTA_TOLERANCE , DEFAULT_PARAM_FPR1_DELTA_TOLERANCE );

    _Fpr1Params.max_scan_dist =
        mig_ut_ini_getfloat ( d , PARAM_FPR1_MAX_SCAN_DISTANCE , DEFAULT_PARAM_FPR1_MAX_SCAN_DISTANCE );

    _Fpr1Params.skip_build3d =
        mig_ut_ini_getfloat ( d , PARAM_FPR1_SKIP_BUILD3D , DEFAULT_PARAM_FPR1_SKIP_BUILD3D );
    
    /* should we check only radius? */
    _Fpr1Params.apply_only_radius = 
    mig_ut_ini_getint ( d, PARAM_FPR1_APPLY_ONLY_RADIUS , DEFAULT_PARAM_FPR1_APPLY_ONLY_RADIUS );
    

    /* 3d object cutting parameters */

    _Fpr1Params.min_max_radius = 
		mig_ut_ini_getfloat ( d, PARAM_FPR1_MIN_MAX_RADIUS , DEFAULT_PARAM_FPR1_MIN_MAX_RADIUS );
    
    _Fpr1Params.max_max_radius = 
		mig_ut_ini_getfloat ( d, PARAM_FPR1_MAX_MAX_RADIUS , DEFAULT_PARAM_FPR1_MAX_MAX_RADIUS );

	_Fpr1Params.max_mean_GL = 
		mig_ut_ini_getfloat ( d, PARAM_FPR1_MAX_MEAN_GL , DEFAULT_PARAM_FPR1_MAX_MEAN_GL );

	_Fpr1Params.min_mean_GL = 
		mig_ut_ini_getfloat ( d, PARAM_FPR1_MIN_MEAN_GL , DEFAULT_PARAM_FPR1_MIN_MEAN_GL );

	_Fpr1Params.max_obj_length = 
        mig_ut_ini_getint ( d , PARAM_FPR1_MAX_OBJ_LENGHT , DEFAULT_PARAM_FPR1_MAX_OBJ_LENGHT );

    _Fpr1Params.max_angle_l2 =
        mig_ut_ini_getfloat ( d , PARAM_FPR1_MAX_ANGLE_L2 , 0.0f );

    _Fpr1Params.max_angle_l3 =
        mig_ut_ini_getfloat ( d , PARAM_FPR1_MAX_ANGLE_L3 , 0.0f );

    _Fpr1Params.max_angle_l4 =
        mig_ut_ini_getfloat ( d , PARAM_FPR1_MAX_ANGLE_L4 , 0.0f );

    _Fpr1Params.max_angle_l5 =
        mig_ut_ini_getfloat ( d , PARAM_FPR1_MAX_ANGLE_L5 , 0.0f );

    _Fpr1Params.max_angle_l6 =
        mig_ut_ini_getfloat ( d , PARAM_FPR1_MAX_ANGLE_L6 , 0.0f );

    _Fpr1Params.max_angle_l7 =
        mig_ut_ini_getfloat ( d , PARAM_FPR1_MAX_ANGLE_L7 , 0.0f );

    _Fpr1Params.max_angle_l8 =
        mig_ut_ini_getfloat ( d , PARAM_FPR1_MAX_ANGLE_L8 , 0.0f );

    _Fpr1Params.max_angle_l9 =
        mig_ut_ini_getfloat ( d , PARAM_FPR1_MAX_ANGLE_L9 , 0.0f );

    _Fpr1Params.max_angle_l10 =
        mig_ut_ini_getfloat ( d , PARAM_FPR1_MAX_ANGLE_L10 , 0.0f );

    _Fpr1Params.max_obj_volume_low_l2 = 
        mig_ut_ini_getfloat ( d , PARAM_FPR1_MAX_OBJ_VOL_LOW_L2 , 0.0f );
    
    _Fpr1Params.max_obj_volume_low_l3 = 
        mig_ut_ini_getfloat ( d , PARAM_FPR1_MAX_OBJ_VOL_LOW_L3 , 0.0f );
    
    _Fpr1Params.max_obj_volume_low_l4 = 
        mig_ut_ini_getfloat ( d , PARAM_FPR1_MAX_OBJ_VOL_LOW_L4 , 0.0f );
    
    _Fpr1Params.max_obj_volume_low_l5 = 
        mig_ut_ini_getfloat ( d , PARAM_FPR1_MAX_OBJ_VOL_LOW_L5 , 0.0f );
    
    _Fpr1Params.max_obj_volume_low_l6 = 
        mig_ut_ini_getfloat ( d , PARAM_FPR1_MAX_OBJ_VOL_LOW_L6 , 0.0f );
    
    _Fpr1Params.max_obj_volume_low_l7 = 
        mig_ut_ini_getfloat ( d , PARAM_FPR1_MAX_OBJ_VOL_LOW_L7 , 0.0f );
    
    _Fpr1Params.max_obj_volume_low_l8 = 
        mig_ut_ini_getfloat ( d , PARAM_FPR1_MAX_OBJ_VOL_LOW_L8 , 0.0f );
    
    _Fpr1Params.max_obj_volume_low_l9 = 
        mig_ut_ini_getfloat ( d , PARAM_FPR1_MAX_OBJ_VOL_LOW_L9 , 0.0f );
    
    _Fpr1Params.max_obj_volume_low_l10 = 
        mig_ut_ini_getfloat ( d , PARAM_FPR1_MAX_OBJ_VOL_LOW_L10 , 0.0f );
    
    _Fpr1Params.max_obj_volume_high_l2 =
        mig_ut_ini_getfloat ( d , PARAM_FPR1_MAX_OBJ_VOL_HIGH_L2 , 0.0f );
    
    _Fpr1Params.max_obj_volume_high_l3 =
        mig_ut_ini_getfloat ( d , PARAM_FPR1_MAX_OBJ_VOL_HIGH_L3 , 0.0f );
    
    _Fpr1Params.max_obj_volume_high_l4 =
        mig_ut_ini_getfloat ( d , PARAM_FPR1_MAX_OBJ_VOL_HIGH_L4 , 0.0f );
    
    _Fpr1Params.max_obj_volume_high_l5 =
        mig_ut_ini_getfloat ( d , PARAM_FPR1_MAX_OBJ_VOL_HIGH_L5 , 0.0f );
    
    _Fpr1Params.max_obj_volume_high_l6 =
        mig_ut_ini_getfloat ( d , PARAM_FPR1_MAX_OBJ_VOL_HIGH_L6 , 0.0f );
    
    _Fpr1Params.max_obj_volume_high_l7 =
        mig_ut_ini_getfloat ( d , PARAM_FPR1_MAX_OBJ_VOL_HIGH_L7 , 0.0f );
    
    _Fpr1Params.max_obj_volume_high_l8 =
        mig_ut_ini_getfloat ( d , PARAM_FPR1_MAX_OBJ_VOL_HIGH_L8 , 0.0f );
    
    _Fpr1Params.max_obj_volume_high_l9 =
        mig_ut_ini_getfloat ( d , PARAM_FPR1_MAX_OBJ_VOL_HIGH_L9 , 0.0f );
    
    _Fpr1Params.max_obj_volume_high_l10 =
        mig_ut_ini_getfloat ( d , PARAM_FPR1_MAX_OBJ_VOL_HIGH_L10 , 0.0f );
    
	/* results */
	_Fpr1Params.dir_results = 
	mig_ut_ini_getstring ( d , PARAM_CAD_DIR_OUT , DEFAULT_PARAM_CAD_DIR_OUT );


    /* log parameters */
    if ( _log.getLogLevel() <= INFO_LOG_LEVEL )
    {
        std::stringstream os;
		os << "\nFPR1 3D object building parameters : ";
        os << "\n\tdelta tollerance  : " << _Fpr1Params.delta_tol;
        os << "\n\tmax scan distance : " << _Fpr1Params.max_scan_dist;
        os << "\nFPR1 3D object cutting parameters : ";
        os << "\n\tmax object lenght   : " << _Fpr1Params.max_obj_length;
        os << "\n\tmax angle l2        : " << _Fpr1Params.max_angle_l2;
        os << "\n\tmax angle l3        : " << _Fpr1Params.max_angle_l3;
        os << "\n\tmax angle l4        : " << _Fpr1Params.max_angle_l4;
        os << "\n\tmax angle l5        : " << _Fpr1Params.max_angle_l5;
        os << "\n\tmax angle l6        : " << _Fpr1Params.max_angle_l6;
        os << "\n\tmax angle l7        : " << _Fpr1Params.max_angle_l7;
        os << "\n\tmax angle l8        : " << _Fpr1Params.max_angle_l8;
        os << "\n\tmax angle l9        : " << _Fpr1Params.max_angle_l9;
        os << "\n\tmax angle l9        : " << _Fpr1Params.max_angle_l10;
        os << "\n\tmax volume low l2   : " << _Fpr1Params.max_obj_volume_low_l2;
        os << "\n\tmax volume low l3   : " << _Fpr1Params.max_obj_volume_low_l3;
        os << "\n\tmax volume low l4   : " << _Fpr1Params.max_obj_volume_low_l4;
        os << "\n\tmax volume low l5   : " << _Fpr1Params.max_obj_volume_low_l5;
        os << "\n\tmax volume low l6   : " << _Fpr1Params.max_obj_volume_low_l6;
        os << "\n\tmax volume low l7   : " << _Fpr1Params.max_obj_volume_low_l7;
        os << "\n\tmax volume low l8   : " << _Fpr1Params.max_obj_volume_low_l8;
        os << "\n\tmax volume low l9   : " << _Fpr1Params.max_obj_volume_low_l9;
        os << "\n\tmax volume low l10  : " << _Fpr1Params.max_obj_volume_low_l10;
        os << "\n\tmax volume high l2  : " << _Fpr1Params.max_obj_volume_high_l2;
        os << "\n\tmax volume high l3  : " << _Fpr1Params.max_obj_volume_high_l3;
        os << "\n\tmax volume high l4  : " << _Fpr1Params.max_obj_volume_high_l4;
        os << "\n\tmax volume high l5  : " << _Fpr1Params.max_obj_volume_high_l5;
        os << "\n\tmax volume high l6  : " << _Fpr1Params.max_obj_volume_high_l6;
        os << "\n\tmax volume high l7  : " << _Fpr1Params.max_obj_volume_high_l7;
        os << "\n\tmax volume high l8  : " << _Fpr1Params.max_obj_volume_high_l8;
        os << "\n\tmax volume high l9  : " << _Fpr1Params.max_obj_volume_high_l9;
        os << "\n\tmax volume high l10 : " << _Fpr1Params.max_obj_volume_high_l10;
        LOG4CPLUS_INFO ( _log , os.str() );
    }

    LOG4CPLUS_DEBUG ( _log , " libmigfpr_1 -> mig_init done..." );
    return MIG_OK;
}

/*
******************************************************************************
*                               DLL MAIN FUNCTION
******************************************************************************
*/

int
mig_run ()
{
    int rc , rc1, rc2;

	char path[MAX_PATH];
    
	LOG4CPLUS_DEBUG ( _log , " libmigfpr1 -> mig_run starting..." );
	
	/******************************************************************************/
    /* try loading fpr1 point from disc */
    
    /* right lung */
    snprintf ( path , MAX_PATH , "%s%c%s_%s_%s_fpr1_r.txt" ,
                        _Fpr1Params.dir_results , MIG_PATH_SEPARATOR ,
                        _CadData->dicom_data.patient_id ,
                        _CadData->dicom_data.study_uid ,
                        _CadData->dicom_data.series_uid );

    LOG4CPLUS_INFO( _log , " Trying to load fpr1 results for right lung from : " << path );
    rc1 = mig_tag_read ( path , &( _CadData->det_r ) );
    if ( rc1 == MIG_OK )
    {
        _CadData->det_r._free = &free;
        LOG4CPLUS_INFO ( _log , " Loaded right lung fpr1 data from disk...");
    }
    
    /* left lung */
    snprintf ( path , MAX_PATH , "%s%c%s_%s_%s_fpr1_l.txt" ,
               _Fpr1Params.dir_results , MIG_PATH_SEPARATOR ,
               _CadData->dicom_data.patient_id ,
               _CadData->dicom_data.study_uid ,
               _CadData->dicom_data.series_uid );

    LOG4CPLUS_INFO( _log , " Trying to load fpr1 results for left lung from : " << path );
    rc2 = mig_tag_read ( path , &( _CadData->det_l ) );
    if ( rc2 == MIG_OK )
    {
        _CadData->det_l._free = &free;
        LOG4CPLUS_INFO ( _log , " Loaded left lung fpr1 data from disk...");
    }
    
    if ( ( rc1 == MIG_OK ) && ( rc2 == MIG_OK ) )
    {
		_CadData ->det_cleanup  = &free;
		LOG4CPLUS_INFO ( _log , " Using fpr1 data from disk...");
        return MIG_OK;
    }

    LOG4CPLUS_INFO( _log , " Could not load fpr1 data from disk. Performing full fpr1..." );


    /* setup input to detection threads */
    _Fpr1Params.thread_data[0].id      = 0;
    _Fpr1Params.thread_data[0].Input   = &( _CadData->det_r );

    _Fpr1Params.thread_data[1].id      = 1;
    _Fpr1Params.thread_data[1].Input   = &( _CadData->det_l );

    /* spawn fpr1 thread 0 */
    rc = pthread_create ( &( _Fpr1Params.thread[0] ) , NULL ,
            &( _fpr1_thread_routine ),  &( _Fpr1Params.thread_data[0] ) );
    if ( rc != 0 )
    {
        LOG4CPLUS_FATAL ( _log , " libmigfpr1 -> mig_run pthread_create returned : " << rc );
        return MIG_ERROR_THREAD;
    }
    
    /* spawn fpr1 thread 1 */
    rc = pthread_create ( &( _Fpr1Params.thread[1] ) , NULL ,
            &( _fpr1_thread_routine ),  &( _Fpr1Params.thread_data[1] ) );
    if ( rc != 0 )
    {
        LOG4CPLUS_FATAL ( _log , " libmigfpr1 -> mig_run pthread_create returned : " << rc );
        return MIG_ERROR_THREAD;
    }

    /* wait for left thread to finish */
    pthread_join ( _Fpr1Params.thread[0] , (void**) &rc );
   
    /* wait for right thread to finish */
    pthread_join ( _Fpr1Params.thread[1] , (void**) &rc );
   

	
	//mig_lst_empty(& _CadData->results);
	
    LOG4CPLUS_DEBUG ( _log , " libmigfpr1 -> mig_run end..." );


    /******************************************************************************/
    /* save fpr1 lists to disk */
    
    /* right lung */
    snprintf ( path , MAX_PATH , "%s%c%s_%s_%s_fpr1_r.txt" ,
                        _Fpr1Params.dir_results , MIG_PATH_SEPARATOR ,
                        _CadData->dicom_data.patient_id ,
                        _CadData->dicom_data.study_uid ,
                        _CadData->dicom_data.series_uid );

    LOG4CPLUS_INFO( _log , " Trying to save fpr1 results for right lung to : " << path );
    rc = mig_tag_write ( path , &( _CadData->det_r ) );
    if ( rc != MIG_OK )
    {
        LOG4CPLUS_WARN( _log , " Could not save fpr1 results for right lung to disk..." );
    }
    else
    {
        LOG4CPLUS_INFO( _log , " Saved fpr1 results for right lung to disk..." );
    }

    /* left lung */
    snprintf ( path , MAX_PATH , "%s%c%s_%s_%s_fpr1_l.txt" ,
                        _Fpr1Params.dir_results , MIG_PATH_SEPARATOR ,
                        _CadData->dicom_data.patient_id ,
                        _CadData->dicom_data.study_uid ,
                        _CadData->dicom_data.series_uid );

    LOG4CPLUS_INFO( _log , " Trying to save fpr1 results for left lung to : " << path );
    rc = mig_tag_write ( path , &( _CadData->det_l ) );
    if ( rc != MIG_OK )
    {
        LOG4CPLUS_WARN( _log , " Could not save fpr1 results for left lung to disk..." );
    }
    else
    {
        LOG4CPLUS_INFO( _log , " Saved fpr1 results for left lung to disk..." );
    }


    
    return MIG_OK;
}

/*
******************************************************************************
*                               DLL CLEANUP
******************************************************************************
*/

void
mig_cleanup ( void* data )
{
    if ( data )
        free ( data );
}

/*
******************************************************************************
*                               DLL INFORMATION
******************************************************************************
*/

void
mig_info ( mig_dll_info_t* info )
{


}

/*
******************************************************************************
*                               PRIVATE PROTOTYPE DEFINITIONS
******************************************************************************
*/

static void*
_fpr1_thread_routine ( void *arg )
{
    fpr1_thread_data *data = (fpr1_thread_data*) arg; /* input parameters */
    mig_lst_t Results = { NULL , NULL , 0 , free };
    
    LOG4CPLUS_DEBUG ( _log , " _fpr1_thread_routine : " << data->id );

    /* build and prune 3d objects */
    if ( mig_im_build_obj3d ( data->Input , &Results ,
            & _build_obj3d_compare ,  & _build_obj3d_select ,
            _CadData->det_cleanup , _Fpr1Params.skip_build3d) != 0 )
    {
        pthread_exit( (void*)MIG_ERROR_MEMORY );
    }
    /* copy local list to global cad data structure */

	//maybe we need to clean data->Input first... NO! it's emptied in mig_im_build_obj3d
	mig_lst_cat ( &Results , data->Input );

    LOG4CPLUS_INFO ( _log , "thread id : " << data->id << " Number of 3D regions : " << mig_lst_len( data->Input ) );
	
    return ( (void*)MIG_OK );
}

/*******************************************************************************/

/* return 0 if and only if a and b are "close" enough */
static int 
_build_obj3d_compare ( const void *a , const void *b )
{
    mig_im_region_t *reg_a = (mig_im_region_t*)a;
    mig_im_region_t *reg_b = (mig_im_region_t*)b;

    /* calcualte z distance */
    if ( fabs( reg_a->centroid[2] - reg_b->centroid[2] ) > _Fpr1Params.max_scan_dist )
        return -1;
        
    if ( ( MIG_POW2( reg_a->centroid[0] - reg_b->centroid[0] ) + 
           MIG_POW2( reg_a->centroid[1] - reg_b->centroid[1] ) ) > 
           MIG_POW2( _Fpr1Params.delta_tol ) )
        return -1;

    return 0;
}

/*******************************************************************************/

/* return 0 if and only if region a should be eliminated */
static int
_build_obj3d_select  ( const void *a )
{
    int length;
    float volume;
    float angle;
	float max_radius;
	float mean_GL;

    mig_im_region_t *reg = (mig_im_region_t*)a;

//DEBUG: returning 1, I need no pruning now (to check next step)
//	return 1;

    printf("before computing radius, reg->size = %d \n", reg->size);
    
    /* calculate region max_radius in mm */
	max_radius = _obj3d_max_radius (reg);
    printf("radius: %f\n", max_radius);
        
	if (max_radius < _Fpr1Params.min_max_radius)
		return 0;

    if (max_radius > _Fpr1Params.max_max_radius)
        return 0;

    if (_Fpr1Params.apply_only_radius)
        return 1; 

	mean_GL = _obj3d_mean_GL (reg);
	if (mean_GL >  _Fpr1Params.max_mean_GL || mean_GL < _Fpr1Params.min_mean_GL)
		return 0;

    /* calculate region lenght in z */
    length = mig_lst_len( &(reg->objs) );
    
    /* calculate region volume */
    volume = _obj3d_volume ( reg );

    /* calculate region angle in degrees */
    angle = _obj3d_angle ( reg );


    /* check wether current region does not satisfy inclusion criteria */
    switch (length)
    {
        case 1 :
            
            return 0;
            break;

        case 2 :
            
            if ( angle > _Fpr1Params.max_angle_l2 )
                return 0;
            
            if ( ( volume < _Fpr1Params.max_obj_volume_low_l2  ) ||
                 ( volume > _Fpr1Params.max_obj_volume_high_l2 ) )
                 return 0;
            
            break;

        case 3 :
            
            if ( angle > _Fpr1Params.max_angle_l3 )
                return 0;
            
            if ( ( volume < _Fpr1Params.max_obj_volume_low_l3  ) ||
                 ( volume > _Fpr1Params.max_obj_volume_high_l3 ) )
                 return 0;
            
            break;

        case 4 :
            
            if ( angle > _Fpr1Params.max_angle_l4 )
                return 0;
            
            if ( ( volume < _Fpr1Params.max_obj_volume_low_l4  ) ||
                 ( volume > _Fpr1Params.max_obj_volume_high_l4 ) )
                 return 0;
            
            break;

       case 5 :
            
            if ( angle > _Fpr1Params.max_angle_l5 )
                return 0;
            
            if ( ( volume < _Fpr1Params.max_obj_volume_low_l5  ) ||
                 ( volume > _Fpr1Params.max_obj_volume_high_l5 ) )
                 return 0;
            
            break;

       case 6 :
            
            if ( angle > _Fpr1Params.max_angle_l6 )
                return 0;
            
            if ( ( volume < _Fpr1Params.max_obj_volume_low_l6  ) ||
                 ( volume > _Fpr1Params.max_obj_volume_high_l6 ) )
                 return 0;
            
            break;
 
       case 7 :
            
            if ( angle > _Fpr1Params.max_angle_l7 )
                return 0;
            
            if ( ( volume < _Fpr1Params.max_obj_volume_low_l7  ) ||
                 ( volume > _Fpr1Params.max_obj_volume_high_l7 ) )
                 return 0;
            
            break;

        case 8 :
            
            if ( angle > _Fpr1Params.max_angle_l8 )
                return 0;
            
            if ( ( volume < _Fpr1Params.max_obj_volume_low_l8  ) ||
                 ( volume > _Fpr1Params.max_obj_volume_high_l8 ) )
                 return 0;
            
            break;

        case 9 :
            
            if ( angle > _Fpr1Params.max_angle_l9 )
                return 0;
            
            if ( ( volume < _Fpr1Params.max_obj_volume_low_l9  ) ||
                 ( volume > _Fpr1Params.max_obj_volume_high_l9 ) )
                 return 0;
            
            break;

        case 10 :
            
            if ( angle > _Fpr1Params.max_angle_l10 )
                return 0;
            
            if ( ( volume < _Fpr1Params.max_obj_volume_low_l10  ) ||
                 ( volume > _Fpr1Params.max_obj_volume_high_l10 ) )
                 return 0;
            
            break;

        default :

            return 0;
            break;
    }

    return -1;
}

/*******************************************************************************/

static float
_obj3d_volume ( mig_im_region_t *reg )
{
    float volume = 0.0f;
    mig_lst_t *objs = &( reg->objs );
    mig_im_region_t *curr;
    mig_lst_iter it;
   
    mig_lst_iter_get ( &it , objs );
    while ( curr = (mig_im_region_t*) mig_lst_iter_next ( &it ) )
    {
            volume += MIG_POW2( curr->radius * _CadData->stack_s.h_res );
    }

    return volume;
}

/*******************************************************************************/

static float
_obj3d_angle ( mig_im_region_t *reg )
{
    mig_lst_t *objs = &( reg->objs );   /* list of 2d objects belonging to current 3d object */
    mig_im_region_t *curr;              /* current 2d view */
    mig_lst_iter it;                    /* list iterator for all current 2d views */
    float angle = 0.0f;                 /* 3d object orientation in degrees */
    float d;
    float *x = NULL , *y1 = NULL , *y2 = NULL;  /* linear fitting arrays */
    int length , idx = 0;               
    float coeff1[2]  = { 0.0f , 0.0f };         /* linear fitting coefficients */
    float coeff2[2]  = { 0.0f , 0.0f };         /* linear fitting coefficients */
    float x_eval[2]  = { 0.0f , 0.0f };         /* linear fitting evaluation absciss */
    float f1_eval[2] = { 0.0f , 0.0f };         /* linear fitting evaluation ordinates */
    float f2_eval[2] = { 0.0f , 0.0f };         /* linear fitting evaluation ordinates */

    /* 3D lenght of object */
    length = mig_lst_len ( &(reg->objs) );
    
    if ( ( length == 0 ) || ( length == 1 )  )
        return -10.0f;
 
    /* allocate array for x */
    x = (float*) malloc ( length * sizeof(float) );
    if ( x == NULL )
        return -10.0f;

    /* allocate array for y1 */
    y1 = (float*) malloc ( length * sizeof(float) );
    if ( y1 == NULL )
    {
        free ( x );
        return -10.0f;
    }

    /* allocate array for y2 */
    y2 = (float*) malloc ( length * sizeof(float) );
    if ( y2 == NULL )
    {
        free ( x );
        free ( y1 );
        return -10.0f;
    }

    /* build up corrdinate arrays */
    mig_lst_iter_get ( &it , objs );
    while (  curr = (mig_im_region_t*) mig_lst_iter_next ( &it ) )
    {
        x[idx] = 3.0f * idx;
        y1[idx] = curr->centroid[0];
        y2[idx] = curr->centroid[1];
        idx ++ ;
    }

    mig_math_polyfit_linear ( x , y1 , length , &(coeff1[0]) , &(coeff1[1]) );
    mig_math_polyfit_linear ( x , y2 , length , &(coeff2[0]) , &(coeff2[1]) );

    x_eval[0] = 0;
    x_eval[1] = x[length-1];

    mig_math_polyval_linear ( (float*) &(x_eval) , (float*) &f1_eval , 2 , coeff1[0] , coeff1[1] );
    mig_math_polyval_linear ( (float*) &(x_eval) , (float*) &f2_eval , 2 , coeff2[0] , coeff2[1] );
    
    d = sqrtf ( MIG_POW2( f1_eval[1] - f1_eval[0] ) + MIG_POW2( f2_eval[1] - f2_eval[0] ) );
    angle = ( ( 2.0f * atanf( d / length ) ) * MIG_RPI ) * 90.0f;

    free ( x );
    free ( y1 );
    free ( y2 );
    
    return angle;
}


/* GF 20170302 made it work also for objs without regions (for 3d regs) */
static float
_obj3d_max_radius ( mig_im_region_t *reg )
{

    if ( reg->objs.num == 0 )
    {
        return reg->radius * _CadData->stack_s.h_res;
    } 


    mig_lst_t *objs = &( reg->objs );   /* list of 2d objects belonging to current 3d object */
    
    mig_im_region_t *curr;              /* current 2d view */
    mig_lst_iter it; 
	float radius = 0;
	float curr_radius_mm = 0;

	/* build up coordinate arrays */
    mig_lst_iter_get ( &it , objs );
    while (  curr = (mig_im_region_t*) mig_lst_iter_next ( &it ) )
    {
		curr_radius_mm = curr->radius * _CadData->stack_s.h_res;
        if (curr_radius_mm  > radius) radius = curr_radius_mm;
    }

	return radius;
}

static float
_obj3d_mean_GL ( mig_im_region_t *reg )
{
	mig_lst_t *objs = &( reg->objs );   /* list of 2d objects belonging to current 3d object */
    mig_im_region_t *curr;              /* current 2d view */
    mig_lst_iter it; 
	float mean_GL = 0;
	int countreg = 0;
	/* build up coordinate arrays */
    mig_lst_iter_get ( &it , objs );
    while (  curr = (mig_im_region_t*) mig_lst_iter_next ( &it ) )
    {
		mean_GL += _obj2d_mean_GL(curr);
		countreg++;
    }
	mean_GL /= countreg;
	return mean_GL;
}

static float
_obj2d_mean_GL (mig_im_region_t *reg )
{
	/*AAAA cambiare considerando che si deve usare float [0,1]*/
	Mig16u* slice = _CadData->stack + (int)( reg->centroid[2] ) * _CadData->stack_s.dim;
	
	int diam, radius;
	int i,pixcount;
	float *crop = NULL;
	float *pcrop = NULL;
	float mean_GL;

	/* crop cutting radii */
    radius = (int) ( reg->radius * 2.0f + 0.5f );
        /* crop cutting diameters */
    diam = 2 * radius + 1;
    
	/* memory for crop */
    crop = (float*) malloc ( MIG_POW2( diam ) * sizeof(float) );


	/* cut crop */
    mig_im_bb_cut_2d ( slice  , _CadData->stack_s.w , _CadData->stack_s.h ,
                       (int) ( reg->centroid[0] ) , (int) ( reg->centroid[1] ) , 
                       crop , radius );

	mean_GL = 0;
	pixcount = 0;
	pcrop = crop;
	for ( i = 0; i != MIG_POW2(diam); i++, pcrop++ )
	{
		if ( *pcrop > 0.000001)
		{
			mean_GL += *pcrop;
			pixcount++;
		}
	}
	
	mean_GL /= pixcount;

	return mean_GL;

}

