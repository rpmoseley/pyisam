Known error codes by variant
============================
This file attempts to list all the error codes returned by the various variants of ISAM library
that are currently supported, in a bid to identify those that can be shared in a common codebase.

.. list-table:: Error Message Comparison
   :widths: 20 35 15 15 15
   :header-rows: 1
   * - Name
     - Message
     - IFISAM
     - VBISAM
     - DISAM
   * - EDUPL
     - duplicate record
     - 100
     - 100
     - 500
   * - ENOTOPEN
     - file not open              
     - 101
     - 101
     - 501
   * - EBADARG   
     - illegal argument          
     - 102  
     - 102  
     - 502
   * - EBADKEY   
     - illegal key desciptor     
     - 103  
     - 103  
     - 503
   * - ETOOMANY  
     - too many files open       
     - 104  
     - 104  
     - 504
   * - EBADFILE  
     - bad isam file format      
     - 105  
     - 105  
     - 505
   * - ENOTEXCL  
     - non-exclusive access      
     - 106  
     - 106  
     - 506
   * - ELOCKED   
     - record locked             
     - 107  
     - 107  
     - 507
   * - EKEXISTS  
     - key already exists        
     - 108  
     - 108  
     - 508
   * - EPRIMKEY  
     - is primary key            
     - 109  
     - 109  
     - 509
   * - EENDFILE  
     - end/begin of file         
     - 110  
     - 110  
     - 510
   * - ENOREC    
     - no record found           
     - 111  
     - 111  
     - 511
   * - ENOCURR   
     - no current record         
     - 112  
     - 112  
     - 512
   * - EFLOCKED  
     - file locked               
     - 113  
     - 113  
     - 513
   * - EFNAME    
     - file name too long        
     - 114  
     - 114  
     - 514
   * - ENOLOK    
     - can't create lock file    
     - 115  
     -
     - 515
   * - EBADMEM   
     - can't alloc memory        
     - 116  
     - 116  
     - 516
   * - ELOGREAD  
     - cannot read log rec       
     - 118  
     - 118  
     - 518
   * - EBADLOG   
     - bad log record            
     - 119  
     - 119  
     - 519
   * - ELOGOPEN  
     - cannot open log file      
     - 120  
     - 120  
     - 520
   * - ELOGWRIT  
     - cannot write log rec      
     - 121  
     - 121  
     - 521
   * - ENOTRANS  
     - no transaction            
     - 122  
     - 122  
     - 522
   * - ENOBEGIN  
     - no begin work yet         
     - 124  
     - 124  
     - 524
   * - ENOPRIM   
     - no primary key            
     - 127  
     - 127  
     - 527
   * - ENOLOG    
     - no logging                
     - 128  
     - 128  
     - 528
   * - EROWSIZE  
     - row size too big          
     - 132  
     - 132  
     - 532
   * - EAUDIT    
     - audit trail exists        
     - 133  
     - 133  
     - 533
   * - ENOLOCKS  
     - no more locks             
     - 134  
     - 134  
     - 534
   * - ENOMANU   
     - must be in ISMANULOCK mode
     - 153  
     - 153  
     -     
   * - EDEADTIME 
     - lock timeout expired      
     - 154  
     - 154  
     -     
   * - EINTERUPT 
     - interrupted isam call     
     - 157  
     - 157  
     -     
   * - EBADFORMAT
     - locking or NODESIZE change
     - 171  
     - 171  
     -     
