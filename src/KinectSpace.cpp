#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <cinder/gl/gl.h>
#include <cinder/gl/Texture.h>
#include <cinder/ImageIo.h>
#include "cinder/app/AppBasic.h"
#include "cinder/Vector.h"
#include "cinder/Utilities.h"
#include "cinder/params/Params.h"
#include "cinder/Camera.h"
#include "ParticleController.h"
#include "OscSender.h"
#include <kinect.h>

#define NUM_INITIAL_PARTICLES 100
#define NUM_PARTICLES_TO_SPAWN 15

using namespace ci;
using namespace ci::app;
using namespace std;


class KinectSpaceApp : public AppBasic {
  public:
	void prepareSettings( Settings *settings );
	void keyDown( KeyEvent event );
	void setup();
	void mouseDown( MouseEvent event );	
	void update();
	void draw();
	void shutdown();

	// PARAMS
	params::InterfaceGl	mParams;

	// CAMERA
	CameraPersp			mCam;
	Quatf				mSceneRotation;
	Vec3f				mEye, mCenter, mUp;
	float				mCameraDistance;
	
	// Particles
	ParticleController	mParticleController;
	float				mZoneRadius;
	
	bool				mCentralGravity;
	bool				mFlatten;

	// Global counters
	float mFrameRateApp;
	float mFrameRateDepth;
	float mFrameRateSkeletons;
	float mFrameRateVideo;
	bool mFullScreen;

	// Global flags
	bool mEnabledDepth;
	bool mEnabledDepthPrev;
	bool mEnabledSkeletons;
	bool mEnabledSkeletonsPrev;
	bool mEnabledVideo;
	bool mEnabledVideoPrev;
	bool mCapture;
	bool mCapturePrev;
	bool mRemoveBackground;
	bool mRemoveBackgroundPrev;
	bool mBinaryMode;
	bool mBinaryModePrev;
	bool mInverted;
	bool mInvertedPrev;
	int mSkeletonSize;

	//OSC 
	osc::Sender sender;
	std::string host;
	int port;
	int counter; //no internet
	int counter_limit;

	// used for skeleton data handling
	int lastActive;


private:
	// Kinect 
	Surface8u mSurface;
	Surface8u mVideoSurface;
	vector<Skeleton> mSkeletons;
	int32_t mUserCount;
	Kinect mKinect;
	void drawSegment(const Skeleton & skeleton, const vector<JointName> & joints);

	// - work in progress
	// used for point scallingscaling
	Vec3f mOffset;
	float mScale;
	Vec3f mSpacing;

	// Depth points
	vector<Vec3f> mPoints;
	// - work in progress

	// Skeleton segments
	void defineBody();
	vector<JointName> mBody;
	vector<JointName> mLeftArm;
	vector<JointName> mLeftLeg;
	vector<JointName> mRightArm;
	vector<JointName> mRightLeg;
	vector<vector<JointName> > mSegments;


};

void KinectSpaceApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 1280, 720 );
	settings->setFrameRate( 60.0f );
}


void KinectSpaceApp::setup()
{
	counter=0;
	counter_limit=0;
	
	// global counters
	mFrameRateApp = 0.0f;
	mFrameRateDepth = 0.0f;
	mFrameRateSkeletons = 0.0f;
	mFrameRateVideo = 0.0f;

	//global flags
	mCapture = true;
	mCapturePrev = true;
	mEnabledDepth = true;
	mEnabledDepthPrev = true;
	mEnabledSkeletons = true;
	mEnabledSkeletonsPrev = true;
	mEnabledVideo = false;
	mEnabledVideoPrev = true;
	mRemoveBackground = false;
	mRemoveBackgroundPrev = true;
	mBinaryMode = false;
	mBinaryModePrev = false;
	mInverted = false;
	mInvertedPrev = false;
	mFullScreen = isFullScreen();
	mUserCount = 0;

	// Particles phyisics
	mCentralGravity = true;
	mFlatten		= false;
	mZoneRadius		= 30.0f;


	// SETUP CAMERA
	mCameraDistance = 200.0f;
	mEye			= Vec3f( 0.0f, 0.0f, mCameraDistance );
	mCenter			= Vec3f::zero();
	mUp				= Vec3f::yAxis();
	mCam.setPerspective( 75.0f, getWindowAspectRatio(), 5.0f, 2000.0f );

	// Define drawing body
	defineBody();

	// Set up OpenGL
	//glEnable(GL_BLEND);
	//glEnable(GL_DEPTH_TEST);
	//glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
	//glEnable(GL_POINT_SMOOTH);
	//glPointSize(0.25f);
	//gl::enableAlphaBlending();
	//gl::enableAdditiveBlending();
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	// SETUP PARAMS
	mParams = params::InterfaceGl( "KinectSpace", Vec2i( 220, 500 ) );
	mParams.addParam( "Scene Rotation", &mSceneRotation, "opened=1" );
	mParams.addSeparator();
	mParams.addParam( "Eye Distance", &mCameraDistance, "min=50.0 max=1500.0 step=15.0 keyIncr=s keyDecr=w" );
	mParams.addParam( "Center Gravity", &mCentralGravity );
	mParams.addParam( "Flatten", &mFlatten );
	mParams.addParam( "Zone Radius", &mZoneRadius, "min=10.0 max=100.0 step=1.0 keyIncr=z keyDecr=Z" );
	mParams.addSeparator();
	mParams.addParam("Depth", & mEnabledDepth, "key=d");
	mParams.addParam("Remove background", & mRemoveBackground, "key=b");
	mParams.addParam("Skeletons", & mEnabledSkeletons, "key=k");
	mParams.addParam("Video", & mEnabledVideo, "key=v");
	mParams.addParam("Binary depth mode", & mBinaryMode, "key=w");
	mParams.addParam("Invert binary image", & mInverted, "key=i");
	mParams.addParam("User count", & mUserCount, "", true);
	mParams.addSeparator();
	mParams.addParam("App frame rate", & mFrameRateApp, "", true);
	mParams.addParam("Depth frame rate", & mFrameRateDepth, "", true);
	mParams.addParam("Skeleton frame rate", & mFrameRateSkeletons, "", true);
	mParams.addParam("Video frame rate", & mFrameRateVideo, "", true);
	mParams.addSeparator();
	mParams.addParam("Nr of skeletons", & mSkeletonSize, "", true);

	// CREATE PARTICLE CONTROLLER
	mParticleController.addParticles( NUM_INITIAL_PARTICLES );

	// Start Kinect with isolated depth tracking only
	mKinect.removeBackground();
	mKinect.start();

	// Point scaling (320/240)
	mScale = 2.0f;
	mSpacing.set(1.0f / 320.0f, -1.0f / 240.0f, 1.0f / 255.0f);
	mOffset.set(-0.5f, 0.5f, 0.0f);


	host = "192.168.1.35";
	port = 7000;
	sender1.setup(host, port);

}

void KinectSpaceApp::mouseDown( MouseEvent event )
{
}

void KinectSpaceApp::keyDown( KeyEvent event )
{
	
	// Key on key... (do it like this in the future)
	switch (event.getCode())
	{
	case KeyEvent::KEY_ESCAPE:
		shutdown();
		break;
//	case KeyEvent::KEY_f:
//		setFullScreen(!isFullScreen());
//		break;
	case KeyEvent::KEY_p:
		mParticleController.addParticles( NUM_PARTICLES_TO_SPAWN );
		break;
	}
}

void KinectSpaceApp::update()
{

	// UPDATE CAMERA
	mEye = Vec3f( 0.0f, 0.0f, mCameraDistance );
	mCam.lookAt( mEye, mCenter, mUp );
	gl::setMatrices( mCam );
	gl::rotate( mSceneRotation );


	// - Handle flags
	// Toggle fullscreen
	if (mFullScreen != isFullScreen())
		setFullScreen(mFullScreen);
	
	// - - Kinect
	// Toggle background remove
	if (mRemoveBackground != mRemoveBackgroundPrev)
	{
		mKinect.removeBackground(mRemoveBackground);
		mRemoveBackgroundPrev = mRemoveBackground;
	}

	// Toggle capture
	if (mCapture != mCapturePrev)
	{
		if (mCapture)
		{	
			mKinect.start();
		}
		else
		{
			mKinect.stop();
		}
		mCapturePrev = mCapture;
	}
	if (mEnabledDepth != mEnabledDepthPrev)
	{
		mKinect.enableDepth(mEnabledDepth);
		mEnabledDepthPrev = mEnabledDepth;
	}
	if (mEnabledSkeletons != mEnabledSkeletonsPrev)
	{
		mKinect.enableSkeletons(mEnabledSkeletons);
		mEnabledSkeletonsPrev = mEnabledSkeletons;
	}
	if (mEnabledVideo != mEnabledVideoPrev)
	{
		mKinect.enableVideo(mEnabledVideo);
		mEnabledVideoPrev = mEnabledVideo;
	}

	// Toggle binary mode
	if (mBinaryMode != mBinaryModePrev || 
		mInverted != mInvertedPrev)
	{
		mKinect.enableBinaryMode(mBinaryMode, mInverted);
		mBinaryModePrev = mBinaryMode;
		mInvertedPrev = mInverted;
	}
	// - end Handing flags

	
	// Capture logic from kinect
	if (mKinect.isCapturing())
	{

		// Get latest Kinect data
		if (mKinect.checkNewDepthFrame())
			mSurface = mKinect.getDepth();
		if (mKinect.checkNewSkeletons())
			mSkeletons = mKinect.getSkeletons();
			mSkeletonSize = mKinect.getUserCount();
		if (mKinect.checkNewVideoFrame())
			mVideoSurface = mKinect.getVideo();
		
		// - work in progress - this has to move away from update, it's more like processing
		// Clear point list
		Vec3f position = Vec3f::zero();
		mPoints.clear();

		// Iterate image rows
			// Iterate rows in pixel
					// Add position to point list
				// Shift point
			// Update position
		// - end work in progress
        
		// Update frame rates
		mFrameRateDepth = mKinect.getDepthFrameRate();
		mFrameRateSkeletons = mKinect.getSkeletonsFrameRate();
		mFrameRateVideo = mKinect.getVideoFrameRate();

	}// End kinect is capturing.

	// UPDATE PARTICLE CONTROLLER AND PARTICLES
	mParticleController.applyForceToParticles( mZoneRadius * mZoneRadius );
	if( mCentralGravity ) mParticleController.pullToCenter( mCenter );
	mParticleController.update( mFlatten );

	// get framerate
	mFrameRateApp = getAverageFps();
}

void KinectSpaceApp::draw()
{
	// clear
	gl::clear( Color( 0, 0, 0 ), true );
	gl::enableDepthRead();
	gl::enableDepthWrite();
	

	// do stuff
	// DRAW PARTICLES
	mParticleController.draw();
  
  // draw a grid
  glPushMatrix();
  gl::enableAlphaBlending();
  int j;
  for ( j = -100 ; j <= 100; j = j + 10 )
  {
	  //xy
	  gl::drawLine(Vec3f(j, -100, 0), Vec3f(j, 100, 0));
	  gl::drawLine(Vec3f(-100, j, 0), Vec3f( 100, j, 0));  

	  //yz
	  gl::drawLine(Vec3f(j, 0, -100), Vec3f(j, 0, 100));
	  gl::drawLine(Vec3f(-100, 0, j), Vec3f( 100, 0, j));  

	  //xz
	  gl::drawLine(Vec3f(0, j, -100), Vec3f(0, j, 100));
	  gl::drawLine(Vec3f(0, -100, j), Vec3f(0, 100, j));  
  }
  gl::disableAlphaBlending();
  glPopMatrix();


	//gl::setMatrices(mCam);
	glScalef(50.0f, 50.0f, 50.0f);

	// get kinect skeletons in 3d space
	glPushMatrix();
	
	int who = 1;
	uint32_t i = 0;
	
	int lastNrSkel = 0;
	
	


	for (vector<Skeleton>::const_iterator skeletonIt = mSkeletons.cbegin(); skeletonIt != mSkeletons.cend(); ++skeletonIt, i++)
	{
		

		if (skeletonIt->size() == JointName::NUI_SKELETON_POSITION_COUNT) // very important
		{
			
	

			 if ( mKinect.getUserCount() == 1 ) {  //primu din orice motiv
				 //console() << "-------primu din orice motiv" << std::endl;
				 //console() << toString(i) << std::endl;
				 who = 1;				 
				 lastActive=i; //il tin minte
				 lastNrSkel = mKinect.getUserCount();

				 //console() << "-------lasti" << std::endl;
				 //console() << toString(lastActive) << std::endl;

			 } else if (mKinect.getUserCount() == 2) { // doi din orice motiv
				// console() << "-------a venit al doilea" << std::endl;
				// console() << toString(i) << std::endl;
				///  console() << "-------lasti" << std::endl;
				// console() << toString(lastActive) << std::endl;

 				 if (i == lastActive) { who = 1;
			//	 console() << "-------am gasit sefu dintre cei doi" << std::endl;
			//	 console() << toString(i) << std::endl;
				 }
				 else { who = 2;
			///	 console() << "-------aici nu ajung niciodat" << std::endl;
			///	 console() << toString(i) << std::endl;
				 }
		
				 lastNrSkel = mKinect.getUserCount();
			 } 
			 else {
				 lastNrSkel = mKinect.getUserCount();
			 }

			 console() <<  toString(2.9*(((((skeletonIt->at(NUI_SKELETON_POSITION_HEAD) * Vec3f(1.0f, 1.0f, 1.0f)).x+2)/4 - 0.3)*2.5))) << std::endl;
			
			 if ((who == 1) && ((2.9*(((((skeletonIt->at(NUI_SKELETON_POSITION_HEAD) * Vec3f(1.0f, 1.0f, 1.0f)).x+2)/4 - 0.3)*2.5))) > 0.6) && ((2.9*(((((skeletonIt->at(NUI_SKELETON_POSITION_HEAD) * Vec3f(1.0f, 1.0f, 1.0f)).x+2)/4 - 0.3)*2.5))) < 2.5))	{

			// Set color
			gl::color(mKinect.getUserColor(i));
			
			

			gl::drawSphere(skeletonIt->at(NUI_SKELETON_POSITION_HAND_LEFT) * Vec3f(0.8f, 0.8f, 0.8f), 0.020f, 50);
			gl::drawSphere(skeletonIt->at(NUI_SKELETON_POSITION_HAND_RIGHT) * Vec3f(0.8f, 0.8f, 0.8f), 0.020f, 50);
			gl::drawSphere(skeletonIt->at(NUI_SKELETON_POSITION_KNEE_LEFT) * Vec3f(0.8f, 0.8f, 0.8f), 0.020f, 50);
			gl::drawSphere(skeletonIt->at(NUI_SKELETON_POSITION_KNEE_RIGHT) * Vec3f(0.8f, 0.8f, 0.8f), 0.020f, 50);
			gl::drawSphere(skeletonIt->at(NUI_SKELETON_POSITION_HEAD) * Vec3f(0.8f, 0.8f, 0.8f), 0.020f, 50);
			gl::drawSphere(skeletonIt->at(NUI_SKELETON_POSITION_SHOULDER_LEFT) * Vec3f(0.8f, 0.8f, 0.8f), 0.020f, 50);
			gl::drawSphere(skeletonIt->at(NUI_SKELETON_POSITION_SHOULDER_RIGHT) * Vec3f(0.8f, 0.8f, 0.8f), 0.020f, 50);
			gl::drawSphere(skeletonIt->at(NUI_SKELETON_POSITION_HIP_CENTER) * Vec3f(0.8f, 0.8f, 0.8f), 0.020f, 100);
			
			//console() << toString (((skeletonIt->at(NUI_SKELETON_POSITION_HAND_LEFT) * Vec3f(1.0f, 1.0f, 1.0f)).y + 2)/4) << std::endl;
			//console() << toString((((skeletonIt->at(NUI_SKELETON_POSITION_HAND_LEFT) * Vec3f(1.0f, 1.0f, 1.0f)).y + 2)/4 - 0.3)*2.5) << std::endl;

			// Set address
			osc::Message message;

			message.setAddress("/kinectPuppet" + toString(who) + "/out/handleft");
			message.addFloatArg((((skeletonIt->at(NUI_SKELETON_POSITION_HAND_LEFT) * Vec3f(1.0f, 1.0f, 1.0f)).y +2)/4 - 0.3)*2.5);
			message.setRemoteEndpoint(host, port);
			sender1.sendMessage(message);
			//sender.sendMessage(message);
			message.clear();			

			message.setAddress("/kinectPuppet" + toString(who) + "/out/handright");
			message.addFloatArg((((skeletonIt->at(NUI_SKELETON_POSITION_HAND_RIGHT) * Vec3f(1.0f, 1.0f, 1.0f)).y+2)/4 - 0.3)*2.5);
			message.setRemoteEndpoint(host, port);
			//sender.sendMessage(message);
			sender1.sendMessage(message);
			message.clear();

			
			
			float val;
			if (((((skeletonIt->at(NUI_SKELETON_POSITION_KNEE_LEFT) * Vec3f(1.0f, 1.0f, 1.0f)).y+2)/4 - 0.3)*2.5) > ((((skeletonIt->at(NUI_SKELETON_POSITION_KNEE_RIGHT) * Vec3f(1.0f, 1.0f, 1.0f)).y+2)/4 - 0.3)*2.5)) {
				val = 0.5 + 2.9*(((((skeletonIt->at(NUI_SKELETON_POSITION_KNEE_LEFT) * Vec3f(1.0f, 1.0f, 1.0f)).y+2)/4 - 0.3)*2.5) - ((((skeletonIt->at(NUI_SKELETON_POSITION_KNEE_RIGHT) * Vec3f(1.0f, 1.0f, 1.0f)).y+2)/4 - 0.3)*2.5));
			} else {
				val = 0.5 - 2.9*(((((skeletonIt->at(NUI_SKELETON_POSITION_KNEE_RIGHT) * Vec3f(1.0f, 1.0f, 1.0f)).y+2)/4 - 0.3)*2.5) - ((((skeletonIt->at(NUI_SKELETON_POSITION_KNEE_LEFT) * Vec3f(1.0f, 1.0f, 1.0f)).y+2)/4 - 0.3)*2.5));
			}
			//console() << toString(val) <<std::endl;
			message.setAddress("/kinectPuppet" + toString(who) + "/out/knees");
			message.addFloatArg(val);
			message.setRemoteEndpoint(host, port);
			//sender.sendMessage(message);
			sender1.sendMessage(message);
			message.clear();


		
			if (((((skeletonIt->at(NUI_SKELETON_POSITION_SHOULDER_LEFT) * Vec3f(1.0f, 1.0f, 1.0f)).z+2)/4 - 0.3)*2.5) > ((((skeletonIt->at(NUI_SKELETON_POSITION_SHOULDER_RIGHT) * Vec3f(1.0f, 1.0f, 1.0f)).z+2)/4 - 0.3)*2.5)) {
				val = 0.5 + 2.9*(((((skeletonIt->at(NUI_SKELETON_POSITION_SHOULDER_LEFT) * Vec3f(1.0f, 1.0f, 1.0f)).z+2)/4 - 0.3)*2.5) - ((((skeletonIt->at(NUI_SKELETON_POSITION_SHOULDER_RIGHT) * Vec3f(1.0f, 1.0f, 1.0f)).z+2)/4 - 0.3)*2.5));
			} else {
				val = 0.5 - 2.9*(((((skeletonIt->at(NUI_SKELETON_POSITION_SHOULDER_RIGHT) * Vec3f(1.0f, 1.0f, 1.0f)).z+2)/4 - 0.3)*2.5) - ((((skeletonIt->at(NUI_SKELETON_POSITION_SHOULDER_LEFT) * Vec3f(1.0f, 1.0f, 1.0f)).z+2)/4 - 0.3)*2.5));
			}
			//console() << toString(val) <<std::endl;

			message.setAddress("/kinectPuppet" + toString(who) + "/out/shoulders");
			message.addFloatArg(val);
			message.setRemoteEndpoint(host, port);
			//sender.sendMessage(message);
			sender1.sendMessage(message);
			message.clear();

			if (((((skeletonIt->at(NUI_SKELETON_POSITION_HEAD) * Vec3f(1.0f, 1.0f, 1.0f)).x+2)/4 - 0.3)*2.5) > ((((skeletonIt->at(NUI_SKELETON_POSITION_HIP_CENTER) * Vec3f(1.0f, 1.0f, 1.0f)).x+2)/4 - 0.3)*2.5)) {
				val = 0.5 + 2.9*(((((skeletonIt->at(NUI_SKELETON_POSITION_HEAD) * Vec3f(1.0f, 1.0f, 1.0f)).x+2)/4 - 0.3)*2.5) - ((((skeletonIt->at(NUI_SKELETON_POSITION_HIP_CENTER) * Vec3f(1.0f, 1.0f, 1.0f)).x+2)/4 - 0.3)*2.5));
			} else {
				val = 0.5 - 2.9*(((((skeletonIt->at(NUI_SKELETON_POSITION_HIP_CENTER) * Vec3f(1.0f, 1.0f, 1.0f)).x+2)/4 - 0.3)*2.5) - ((((skeletonIt->at(NUI_SKELETON_POSITION_HEAD) * Vec3f(1.0f, 1.0f, 1.0f)).x+2)/4 - 0.3)*2.5));
			}
			//console() << toString(val) <<std::endl;

			message.setAddress("/kinectPuppet" + toString(who) + "/out/tilt");
			message.addFloatArg(val);
			message.setRemoteEndpoint(host, port);
			//sender.sendMessage(message);
			sender1.sendMessage(message);
			message.clear();

			//counter++;
			//if (counter == (counter_limit + 1)) {
			//	counter = 0;
			//	console() << "reseting counter " << std::endl;
			//}
			 }
			
	
			
		}

		
	}
	glPopMatrix();

	// DRAW PARAMS WINDOW
	params::InterfaceGl::draw();

	// Draw kinect debug windows
	gl::setMatricesWindow(getWindowSize(), true);
	if (mSurface && mEnabledDepth)
	{
		gl::enableAlphaBlending(); //check if this is even sane
		gl::color(Colorf::white());
		gl::draw(gl::Texture(mSurface), Area(0, 0, mSurface.getWidth(), mSurface.getHeight()), Rectf(265.0f, 15.0f, 505.0f, 195.0f));
		gl::disableAlphaBlending(); //like this ?
	}
	if (mVideoSurface && mEnabledVideo)
		gl::draw(gl::Texture(mVideoSurface), Area(0, 0, mVideoSurface.getWidth(), mVideoSurface.getHeight()), Rectf(508.0f, 15.0f, 1284.0f, 570.0f));

}

void KinectSpaceApp::shutdown()
{

	// Stop input
	mKinect.stop();

	// Force exit
	exit(1);

}

// Draw segment
void KinectSpaceApp::drawSegment(const Skeleton & skeleton, const vector<JointName> & joints)
{
	// DO IT!
	for (uint32_t i = 0; i < joints.size() - 1; i++)
	{
		gl::drawLine( skeleton.at(joints[i]) * Vec3f(0.6f, 0.6f, 0.6f), skeleton.at(joints[i + 1]) * Vec3f(0.6f, 0.6f, 0.6f) );
		
	}
}

void KinectSpaceApp::defineBody()
{

	// Bail if defined
	if (mSegments.size() > 0)
		return;
	
	// Body
	mBody.push_back(NUI_SKELETON_POSITION_HIP_CENTER);
	mBody.push_back(NUI_SKELETON_POSITION_SPINE);
	mBody.push_back(NUI_SKELETON_POSITION_SHOULDER_CENTER);
	mBody.push_back(NUI_SKELETON_POSITION_HEAD);

	// Left arm
	mLeftArm.push_back(NUI_SKELETON_POSITION_SHOULDER_CENTER);
	mLeftArm.push_back(NUI_SKELETON_POSITION_SHOULDER_LEFT);
	mLeftArm.push_back(NUI_SKELETON_POSITION_ELBOW_LEFT);
	mLeftArm.push_back(NUI_SKELETON_POSITION_WRIST_LEFT);
	mLeftArm.push_back(NUI_SKELETON_POSITION_HAND_LEFT);

	// Left leg
	mLeftLeg.push_back(NUI_SKELETON_POSITION_HIP_CENTER);
	mLeftLeg.push_back(NUI_SKELETON_POSITION_HIP_LEFT);
	mLeftLeg.push_back(NUI_SKELETON_POSITION_KNEE_LEFT);
	mLeftLeg.push_back(NUI_SKELETON_POSITION_ANKLE_LEFT);
	mLeftLeg.push_back(NUI_SKELETON_POSITION_FOOT_LEFT);

	// Right arm
	mRightArm.push_back(NUI_SKELETON_POSITION_SHOULDER_CENTER);
	mRightArm.push_back(NUI_SKELETON_POSITION_SHOULDER_RIGHT);
	mRightArm.push_back(NUI_SKELETON_POSITION_ELBOW_RIGHT);
	mRightArm.push_back(NUI_SKELETON_POSITION_WRIST_RIGHT);
	mRightArm.push_back(NUI_SKELETON_POSITION_HAND_RIGHT);

	// Right leg
	mRightLeg.push_back(NUI_SKELETON_POSITION_HIP_CENTER);
	mRightLeg.push_back(NUI_SKELETON_POSITION_HIP_RIGHT);
	mRightLeg.push_back(NUI_SKELETON_POSITION_KNEE_RIGHT);
	mRightLeg.push_back(NUI_SKELETON_POSITION_ANKLE_RIGHT);
	mRightLeg.push_back(NUI_SKELETON_POSITION_FOOT_RIGHT);

	// Build skeleton drawing list
	mSegments.push_back(mBody);
	mSegments.push_back(mLeftArm);
	mSegments.push_back(mLeftLeg);
	mSegments.push_back(mRightArm);
	mSegments.push_back(mRightLeg);

}

CINDER_APP_BASIC( KinectSpaceApp, RendererGl )
