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
INCPRE+=$(WPD_CFLAGS) -I..
.ELSE
INCPRE+=$(SOLARVER)$/$(INPATH)$/inc$/libwpd
.ENDIF

.IF "$(SYSTEM_LIBWPG)" == "YES"
INCPRE+=$(WPG_CFLAGS) -I..
.ELSE
INCPRE+=$(SOLARVER)$/$(INPATH)$/inc$/libwpg
.ENDIF

SLOFILES= \
    $(SLO)$/CDRCollector.obj \
    $(SLO)$/CDRDocument.obj \
    $(SLO)$/CDRInternalStream.obj \
    $(SLO)$/CDRParser.obj \
    $(SLO)$/CDRPath.obj \
    $(SLO)$/CDRSVGGenerator.obj \
    $(SLO)$/CDRTypes.obj \
    $(SLO)$/libcdr_utils.obj

LIB1ARCHIV=$(LB)$/libcdrlib.a
LIB1TARGET=$(SLB)$/$(TARGET).lib
LIB1OBJFILES= $(SLOFILES)

.INCLUDE :  target.mk
