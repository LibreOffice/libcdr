EXTERNAL_WARNINGS_NOT_ERRORS := TRUE

PRJ=..$/..$/..$/..$/..$/..

PRJNAME=libcdr
TARGET=cdrlib
ENABLE_EXCEPTIONS=TRUE
LIBTARGET=NO

.INCLUDE :  settings.mk

.IF "$(GUI)$(COM)"=="WNTMSC"
CFLAGS+=-GR
.ENDIF
.IF "$(COM)"=="GCC"
CFLAGSCXX+=-frtti
.ENDIF

.IF "$(SYSTEM_LIBWPD)" == "YES"
INCPRE+=$(WPD_CFLAGS)
.ELSE
INCPRE+=$(SOLARVER)$/$(INPATH)$/inc$/libwpd
.ENDIF

.IF "$(SYSTEM_LIBWPG)" == "YES"
INCPRE+=$(WPG_CFLAGS)
.ELSE
INCPRE+=$(SOLARVER)$/$(INPATH)$/inc$/libwpg
.ENDIF

.IF "$(SYSTEM_LCMS2)" == "YES"
INCPRE+=$(LCMS2_CFLAGS)
.ELSE
INCPRE+=$(SOLARVER)$/$(INPATH)$/inc$/lcms2
.ENDIF

.IF "$(SYSTEM_ZLIB)" != "YES"
INCPRE+=-I$(SOLARVER)$/$(INPATH)$/inc$/external/zlib
.ENDIF

SLOFILES= \
    $(SLO)$/CDRCollector.obj \
    $(SLO)$/CDRDocument.obj \
    $(SLO)$/CDRInternalStream.obj \
    $(SLO)$/CDROutputElementList.obj \
    $(SLO)$/CDRParser.obj \
    $(SLO)$/CDRPath.obj \
    $(SLO)$/CDRStringVector.obj \
    $(SLO)$/CDRSVGGenerator.obj \
    $(SLO)$/CDRTypes.obj \
    $(SLO)$/libcdr_utils.obj

LIB1ARCHIV=$(LB)$/libcdrlib.a
LIB1TARGET=$(SLB)$/$(TARGET).lib
LIB1OBJFILES= $(SLOFILES)

.INCLUDE :  target.mk
