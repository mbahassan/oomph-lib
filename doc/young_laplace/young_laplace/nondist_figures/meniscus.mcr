#!MC 1100



##################################################
# OUTPUT TO PNG (1) OR AVI (0)
##################################################
$!VARSET |PNG|=0


$!VARSET  |lostep|=0
$!VARSET  |dlstep|=1
$!VARSET  |nstep|=11


$!LOOP |nstep|
$!NEWLAYOUT
$!DRAWGRAPHICS FALSE
$!VARSET |istep|=(|lostep|+(|LOOP|-1)*|dlstep|)



$!READDATASET  '"RESLT/soln|istep|.dat" ' 
  READDATAOPTION = NEW
  RESETSTYLE = YES
  INCLUDETEXT = NO
  INCLUDEGEOM = NO
  INCLUDECUSTOMLABELS = NO
  VARLOADMODE = BYNAME
  ASSIGNSTRANDIDS = YES
  INITIALPLOTTYPE = CARTESIAN3D
$!FIELDLAYERS SHOWSHADE = YES
$!FIELDLAYERS SHOWMESH = NO
$!FIELDMAP [1-|NUMZONES|]  EDGELAYER{COLOR = RED}
$!FIELDMAP [1-|NUMZONES|]  EDGELAYER{LINETHICKNESS = 0.02}
$!REDRAWALL 
$!VIEW FIT
$!FIELDLAYERS USETRANSLUCENCY = YES
$!FIELDMAP [1-|NUMZONES|]  SHADE{COLOR = CYAN}

$!GLOBALTHREEDVECTOR UVAR = 7
$!GLOBALTHREEDVECTOR VVAR = 8
$!GLOBALTHREEDVECTOR WVAR = 9
$!FIELDLAYERS SHOWVECTOR = YES
$!RESETVECTORLENGTH 
$!VIEW FIT
$!THREEDVIEW 
  PSIANGLE = 57.8577
  THETAANGLE = -125.715
  ALPHAANGLE = -2.10322
  VIEWERPOSITION
    {
    X = 6.690686862801665
    Y = 4.950958410030791
    Z = 4.840801837665931
    }
$!FIELDMAP [1-|NUMZONES|]  VECTOR{COLOR = BLACK}
$!FIELDMAP [1-|NUMZONES|]  POINTS{IJKSKIP{I = 4}}
$!FIELDMAP [1-|NUMZONES|]  POINTS{IJKSKIP{J = 4}}
$!THREEDAXIS YDETAIL{TITLE{TITLEMODE = USETEXT}}
$!THREEDAXIS YDETAIL{TITLE{TEXT = 'y'}}
$!THREEDAXIS ZDETAIL{TITLE{TITLEMODE = USETEXT}}
$!THREEDAXIS ZDETAIL{TITLE{TEXT = 'z'}}
$!THREEDAXIS XDETAIL{TITLE{TITLEMODE = USETEXT}}
$!THREEDAXIS XDETAIL{TITLE{TEXT = 'x'}}

$!CREATERECTANGULARZONE
  IMAX = 2
  JMAX = 2
  KMAX = 2
  X1 = 0
  Y1 = 0
  Z1 = 0
  X2 = 1
  Y2 = 1
  Z2 = 1
  XVAR = 1
  YVAR = 2
  ZVAR = 3

$!ACTIVEFIELDMAPS += [|NUMZONES|]
$!VIEW FIT
$!THREEDAXIS AXISMODE = XYDEPENDENT
$!VIEW AXISFIT
  AXIS = 'Z'
  AXISNUM = 1
$!THREEDAXIS AXISMODE = XYZDEPENDENT
$!VIEW AXISFIT
  AXIS = 'Y'
  AXISNUM = 1
$!VIEW AXISFIT
  AXIS = 'X'
  AXISNUM = 1
$!THREEDAXIS DEPXTOZRATIO = 1
$!VIEW FIT
$!ACTIVEFIELDMAPS -= [|NUMZONES|]



$!PAPER SHOWPAPER=YES
$!DRAWGRAPHICS TRUE
$!REDRAWALL



##########################################################
# OUTPUT
##########################################################
$!IF |PNG|==1

        $!EXPORTSETUP EXPORTFORMAT = PNG
        $!EXPORTSETUP IMAGEWIDTH = 600
        $!EXPORTSETUP EXPORTFNAME = 'meniscus|LOOP|.png'
        $!EXPORT
          EXPORTREGION = ALLFRAMES

$!ELSE

        $!IF |LOOP|>1
                $!EXPORTNEXTFRAME
        $!ELSE

                $!EXPORTSETUP
                 EXPORTFORMAT = AVI
                 EXPORTFNAME = "meniscus.avi"
                $!EXPORTSETUP IMAGEWIDTH = 829
                $!EXPORTSTART
                  EXPORTREGION = ALLFRAMES
       $!ENDIF

$!ENDIF



##########################################################
# END OF LOOP OVER STEPS
##########################################################
$!ENDLOOP




$!IF |PNG|==0
        $!EXPORTFINISH
$!ENDIF


