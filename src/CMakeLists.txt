<<<<<<< HEAD
# rtl_433 is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# rtl_433 is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.

########################################################################
# Build utility
########################################################################
add_executable(rtl_433
	baseband.c
	bitbuffer.c
	data.c
	pulse_demod.c
	pulse_detect.c
	rtl_433.c
	util.c
	devices/acurite.c
	devices/alecto.c
	devices/ambient_weather.c
	devices/blyss.c
	devices/brennenstuhl_rcs_2044.c
	devices/calibeur.c
	devices/cardin.c
	devices/chuango.c
	devices/current_cost.c
	devices/danfoss.c
	devices/dsc.c
	devices/ec3k.c
	devices/efergy_e2_classic.c
	devices/elv.c
	devices/emontx.c
	devices/esperanza_ews.c
	devices/fineoffset.c
	devices/fineoffset_wh1080.c
	devices/generic_remote.c
	devices/generic_temperature_sensor.c
	devices/gt_wt_02.c
	devices/hideki.c
	devices/ht680.c
	devices/inovalley-kw9015b.c
	devices/intertechno.c
	devices/kedsum.c
	devices/lacrosse.c
	devices/lacrosse_TX141TH_Bv2.c
	devices/lacrossews.c
	devices/lightwave_rf.c
	devices/mebus.c
	devices/newkaku.c
	devices/nexus.c
	devices/oil_watchman.c
	devices/oregon_scientific.c
	devices/oregon_scientific_v1.c
	devices/prologue.c
	devices/rubicson.c
	devices/silvercrest.c
	devices/springfield.c
	devices/steffen.c
	devices/tfa_twin_plus_30.3049.c
	devices/tfa_pool_thermometer.c
	devices/valeo.c
	devices/waveman.c
	devices/wg_pb12v1.c
	devices/wt450.c
	devices/x10_rf.c
	devices/s3318p.c
	devices/akhan_100F14.c
	devices/quhwa.c
	devices/proove.c
	devices/bresser_3ch.c
	devices/oregon_scientific_sl109h.c
	devices/steelmate.c
	devices/schraeder.c
	devices/elro_db286a.c
	devices/efergy_optical.c
	devices/hondaremote.c
	devices/new_template.c
	devices/radiohead_ask.c
	devices/kerui.c
	devices/fineoffset_wh1050.c
	devices/honeywell.c
	devices/maverick_et73x.c
	devices/rftech.c
	devices/lacrosse_tx35.c
	devices/vaillant_vrt340f.c
	devices/ibis_beacon.c
	devices/oil_standard.c
	devices/tpms_citroen.c

)

add_library(data data.c)

target_link_libraries(rtl_433
	${LIBRTLSDR_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)


set(INSTALL_TARGETS rtl_433)
if(UNIX)
target_link_libraries(rtl_433 m)
endif()

########################################################################
# Install built library files & utilities
########################################################################
install(TARGETS ${INSTALL_TARGETS}
    RUNTIME DESTINATION bin              # .dll file
)
=======
########################################################################
# Build libraries and executables
########################################################################
add_executable(rtl_433
	abuf.c
	am_analyze.c
	baseband.c
	bitbuffer.c
	compat_paths.c
	compat_time.c
	confparse.c
	data.c
	decoder_util.c
	fileformat.c
	list.c
	mongoose.c
	optparse.c
	output_mqtt.c
	pulse_demod.c
	pulse_detect.c
	r_api.c
	r_util.c
	rtl_433.c
	samp_grab.c
	sdr.c
	term_ctl.c
	util.c
	devices/acurite.c
	devices/akhan_100F14.c
	devices/alecto.c
	devices/ambient_weather.c
	devices/ambientweather_tx8300.c
	devices/ambientweather_wh31e.c
	devices/blyss.c
	devices/brennenstuhl_rcs_2044.c
	devices/bresser_3ch.c
	devices/bresser_5in1.c
	devices/bt_rain.c
	devices/calibeur.c
	devices/cardin.c
	devices/chuango.c
	devices/companion_wtr001.c
	devices/current_cost.c
	devices/danfoss.c
	devices/digitech_xc0324.c
	devices/dish_remote_6_3.c
	devices/dsc.c
	devices/efergy_e2_classic.c
	devices/efergy_optical.c
	devices/elro_db286a.c
	devices/elv.c
	devices/emontx.c
	devices/esa.c
	devices/esperanza_ews.c
	devices/fineoffset.c
	devices/fineoffset_wh1050.c
	devices/fineoffset_wh1080.c
	devices/flex.c
	devices/fordremote.c
	devices/fs20.c
	devices/ft004b.c
	devices/ge_coloreffects.c
	devices/generic_motion.c
	devices/generic_remote.c
	devices/generic_temperature_sensor.c
	devices/gt_wt_02.c
	devices/hideki.c
	devices/hondaremote.c
	devices/honeywell.c
	devices/honeywell_wdb.c
	devices/ht680.c
	devices/ibis_beacon.c
	devices/infactory.c
	devices/inovalley-kw9015b.c
	devices/interlogix.c
	devices/intertechno.c
	devices/kedsum.c
	devices/kerui.c
	devices/lacrosse.c
	devices/lacrosse_TX141TH_Bv2.c
	devices/lacrosse_tx35.c
	devices/lacrosse_ws7000.c
	devices/lacrossews.c
	devices/lightwave_rf.c
	devices/m_bus.c
	devices/maverick_et73.c
	devices/maverick_et73x.c
	devices/mebus.c
	devices/new_template.c
	devices/newkaku.c
	devices/nexa.c
	devices/nexus.c
	devices/oil_standard.c
	devices/oil_watchman.c
	devices/opus_xt300.c
	devices/oregon_scientific.c
	devices/oregon_scientific_sl109h.c
	devices/oregon_scientific_v1.c
	devices/philips.c
	devices/prologue.c
	devices/proove.c
	devices/quhwa.c
	devices/radiohead_ask.c
	devices/rftech.c
	devices/rubicson.c
	devices/s3318p.c
	devices/schraeder.c
	devices/silvercrest.c
	devices/simplisafe.c
	devices/smoke_gs558.c
	devices/solight_te44.c
	devices/springfield.c
	devices/steelmate.c
	devices/tfa_pool_thermometer.c
	devices/tfa_twin_plus_30.3049.c
	devices/thermopro_tp11.c
	devices/thermopro_tp12.c
	devices/tpms_citroen.c
	devices/tpms_ford.c
	devices/tpms_jansite.c
	devices/tpms_pmv107j.c
	devices/tpms_renault.c
	devices/tpms_toyota.c
	devices/ts_ft002.c
	devices/ttx201.c
	devices/vaillant_vrt340f.c
	devices/waveman.c
	devices/wg_pb12v1.c
	devices/wssensor.c
	devices/wt0124.c
	devices/wt450.c
	devices/x10_rf.c
	devices/x10_sec.c
)

if("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
	SET_SOURCE_FILES_PROPERTIES(mongoose.c PROPERTIES COMPILE_FLAGS "-Wno-format")
endif()
if("${CMAKE_C_COMPILER_ID}" MATCHES "Clang")
	SET_SOURCE_FILES_PROPERTIES(mongoose.c PROPERTIES COMPILE_FLAGS "-Wno-format-pedantic -Wno-large-by-value-copy")
endif()

if(MSVC)
	# needs CMake 3.1 but Windows builds should have that
	target_sources(rtl_433 PRIVATE getopt/getopt.c)
endif()

add_library(data data.c abuf.c term_ctl.c mongoose.c)
target_link_libraries(data ${NET_LIBRARIES})

target_link_libraries(rtl_433
	${SDR_LIBRARIES}
	${NET_LIBRARIES}
)

set(INSTALL_TARGETS rtl_433)
if(UNIX)
target_link_libraries(rtl_433 m)
endif()

# Explicitly say that we want C99
set_property(TARGET rtl_433 PROPERTY C_STANDARD 99)

########################################################################
# Install built library files & utilities
########################################################################
install(TARGETS ${INSTALL_TARGETS}
    RUNTIME DESTINATION bin              # .dll file
)
>>>>>>> e5a6083a0a1677f4b0a435602fc623fbbb54ecc3
