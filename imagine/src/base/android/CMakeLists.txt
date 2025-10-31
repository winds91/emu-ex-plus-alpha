addPkgEgl(imagine)
addPkgConfigLibs(imagine "-landroid")

target_sources(
	imagine PRIVATE
	Application.cc
	ApplicationContext.cc
	AndroidWindow.cc
	AndroidScreen.cc
	ALooperEventLoop.cc
	AndroidGLContext.cc
	FrameTimer.cc
	HardwareBuffer.cc
	inputConfig.cc
	TextField.cc
	input.cc
	moga.cc
	PerformanceHintManager.cc
	Sensor.cc
	surfaceTexture.cc
	RootCpufreqParamSetter.cc
	VibrationManager.cc
	privateApi/libhardware.c
	privateApi/GraphicBuffer.cc
	../common/timer/TimerFD.cc
	../common/eventloop/FDCustomEvent.cc
	../common/PosixPipe.cc
	../common/EGLContextBase.cc
	../common/SimpleFrameTimer.cc
	../../util/jni.cc
)

if(android_ndkSDK LESS_EQUAL 16)
	# add missing libc functions when compiling with newer NDK headers
	target_sources(imagine PRIVATE compat.c)
endif()
