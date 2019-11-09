# BForChapel
Blame tool for Chapel programs 

# Default branch (instructions based on)
BFC-ml-llvm3.3-chpl1.15


=========================
INSTALL
=========================
NOTE:
    1. The only tested platform is https://www.glue.umd.edu/hpcc/dt2.html
    2. The build and run were done with dependencies' specific versions, not guaranteed to work with other versions (though for most third-party libraries, it should be backward compatible, except for Chapel and LLVM)
    3. Some scripts might need to be adjusted to your own environment
    4. The work is not in a good shape for release, so the installation and running steps could be frustrating :(

$BFC_ROOT - Top level of chplBlamer source
$CHPL_HOME - Top level of Chapel source

1) LLVM Static analysis (Needs to run as a LLVM pass) 
(BFC_LLVM_Static_Analysis)
------------------------------------------------------------
Dependencies:
	LLVM 3.3
	Chapel 1.15 
-----------------------------------------------------------

a) Download Chapel 1.15 source code https://github.com/chapel-lang/chapel.git
   Download LLVM 3.3 source code 
   Download all my attached scripts
   Download chplBlamer https://github.com/hzhang86/BForChapel.git and swtich to branch BFC-ml-llvm3.3-chpl1.15

b) Build Chapel with llvm
   ==> Put llvm-3.3.src.tar.gz and cfe-3.3.src.tar.gz under $CHPL_HOME/third-party/llvm and modify the corresponding unpack-llvm.sh to unpack and install llvm 3.3 instead of 3.7
   ==> Put setEnvBeforeBuild.sh and setEnvForMultiLocaleBeforeRun.sh under $CHPL_HOME
   ==> Replace the whole runtime directory under $CHPL_HOME with Chpl_Runtime_Modified in ChplBlamer source
   ==> source setEnvBeforeBuild.sh
   ==> export REQUIRES_RTTI=1 (used to enable rtti when built llvm with boost)
   ==> Download and install boost 1.65, libunwind 1.12, PAPI 5.5 and set the roots as $BOOST_INSTALL, $LIBUNWIND_INSTALL, $PAPI_INSTALL
   ==> make
   ==> make install

c) Build custom blame pass 
	In $CHPL_HOME/third-party/llvm/llvm/Makefile.rules, change:
		CPP.Flags     += $(sort -I$(PROJ_OBJ_DIR) -I$(PROJ_SRC_DIR) \ 
		to 
		CPP.Flags     += $(sort -I$(PROJ_OBJ_DIR) -I$(PROJ_SRC_DIR) -I$BOOST_INSTALL \
	==> cp -r $BFC_ROOT/BFC_StaticAnalysis $CHPL_HOME/third-party/llvm/llvm/lib/Transforms/BFC
	==> cp -r $BFC_ROOT/BFC_StaticAnalysis $CHPL_HOME/third-party/llvm/build/linux64-gnu/lib/Transforms/BFC
    ==> cd $CHPL_HOME/third-party/llvm/build/linux64-gnu/lib/Transforms/BFC
	==> make
	==> make install (if you set the llvm's PREFIX_DIR)


2) Runtime Data Collection
--------------------------------------------------
Dependencies:
    DyninstAPI 9.2.0
a) Download Dyninst 9.2.0 and build from source, set $DYNINST_INSTALL

e) Create Monitor
	==> cd $BFC_ROOT/BFC_Monitor
	==> ./build.sh 
		// (Output: monitor)


3) Post Processing
a) Build address parser
	==> cd $BFC_ROOT/BFC_PostMortem_1_ml
	==> ./compile-AP.sh 		
		// (Output: addParser)

b) Build post-mortem parser
	==> cd $BFC_ROOT/BFC_PostMortem_2_ml
	==> ./compileAlt.sh 
		// (Output: AltParser)


4) GUI (and basic data aggregation)
(BFC_GUI)
-------------------------------------
Dependencies:
	ant (optional)
------------------------------------

a) Build GUI from Java source
	==> cd $BFC_ROOT/BFC_GUI
	==> ant

5) Environment Variables
--------------------------------------
	$DYNINST_INSTALL
	$BFC_ROOT/BFC_Monitor (optional)
	$BFC_ROOT/BFC_PostMortem_1_ml (optional)
	$BFC_ROOT/BFC_PostMortem_2_ml (optional)
	
LD_LIBRARY_PATH:
	$PAPI_INSTALL/lib    
	$BOOST_INSTALL/lib
	$DYNINST_INSTALL/lib
	$LIBUNWIND_INSTALL/lib
	
=========================
END INSTALL
=========================



=========================
RUN (using HPL as an example)
=========================
Step 1: LLVM Static Analysis
-------------------------------
a) Build program you want to analyze into LLVM bitcode
    ==> source setEnvForMultiLocaleBeforeRun.sh
    ==> chpl -g --no-checks --llvm --savec=./ hpl.chpl -o hpl

b) Create output directories in same directory as <name>.bc
	(should probably automate this step)
	==> mkdir ./DOT/      /* Debug directory with graphical representation of blame for each function */
	==> mkdir ./EXPORT/   /* Directory where the meat of analysis is placed */
	==> mkdir ./OUTPUT/    /* Raw debug output for user modules, listed per analyzed function */
	==> mkdir ./OUTPUT/MODULES    /* Raw debug output for chapel internal modules, listed per analyzed function */
	
c) Run custom LLVM blame pass on that bitcode 
	/* This will populate the directories you created above */
    ==> opt -load $CHPL_HOME/third-party/llvm/build/linux64-gnu/Release+Debug+Asserts/lib/BFC.so -bfc chpl__module.bc
	

Step 2: Run instrumented program on a cluster and do address parsing concurrently
---------------------------------
    ==> mkdir ./COMPUTE /*parsed compute stack traces*/
    ==> mkdir ./PRESPAWN /*parsed prespawn stack traces*/
    ==> mkdir ./FORK /*parsed fork stack traces*/
	==> sbatch ibv-job.sh
	// This will create a compute file with the hostname of every host it is run on
	// For a single host run, one file will be created
    // The raw stack traces will be parsed and populated into above 3 folders


Step 3: Run post-mortem analysis
----------------------------------
    ==> AltParser hpl_real post_process_config.txt
	 // it will output a file called PARSED_host name
 	 // config file of form:
		<number of .blm files>
		<series of .blm files>
		<number of .struct files>
		<series of .struct files>
		<number of .se flies>
		<series of .se files>


Step 4: Launch GUI
----------------------------------
    ==> java -Xms512m -Xmx1024m -cp $BFC_ROOT/BFC_GUI blame/MainGUI gui_config.txt
	 // config file of form:
		<0,1>: 0 - Classic BFC, 1 - Skid analysis
		<Name of test>
        <absolute path to 'usr_names'>
		<number of nodes to be imported> 	
		<absolute path to PARSED_'host name' file1>
		<absolute path to PARSED_'host name' file2>
        <absolute path to PARSED_'host name' file3>
		...

=========================
END RUN
=========================


