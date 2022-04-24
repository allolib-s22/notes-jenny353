/*
Allocore Example: pickRay

Description:
The example demonstrates how to interact with objects using ray intersection
tests.

Author:
Tim Wood, 9/19/2014
*/

#include "al/app/al_App.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/math/al_Random.hpp"
#include "al/math/al_Ray.hpp"

#include "Gamma/Analysis.h"
#include "Gamma/Effects.h"
#include "Gamma/Envelope.h"
#include "Gamma/Oscillator.h"

#include "al/scene/al_PolySynth.hpp"
#include "al/scene/al_SynthSequencer.hpp"
#include "al/ui/al_ControlGUI.hpp"
#include "al/ui/al_Parameter.hpp"
#include "al/io/al_Window.hpp"

using namespace al;

#define N 1000


int note = 72;


class Sound : public SynthVoice {
 public:
  // Unit generators
  gam::Pan<> mPan;
  gam::Sine<> mOsc;
  gam::Env<3> mAmpEnv;
  // envelope follower to connect audio output to graphics
  gam::EnvFollow<> mEnvFollow;

  // Additional members
  Mesh mMesh;


  // Initialize voice. This function will only be called once per voice when
  // it is created. Voices will be reused if they are idle.
  void init() override {
    // Intialize envelope
    mAmpEnv.curve(0);  // make segments lines
    mAmpEnv.levels(0, 1, 1, 0);
    mAmpEnv.sustainPoint(2);  // Make point 2 sustain until a release is issued

    // We have the mesh be a sphere
    addSphere(mMesh, 0.02, 30);

    // This is a quick way to create parameters for the voice. Trigger
    // parameters are meant to be set only when the voice starts, i.e. they
    // are expected to be constant within a voice instance. (You can actually
    // change them while you are prototyping, but their changes will only be
    // stored and aplied when a note is triggered.)
 
    //(default val, min, max)
    createInternalTriggerParameter("amplitude", 0.3, 0.0, 1.0);
    createInternalTriggerParameter("frequency", 60, 20, 5000);
    createInternalTriggerParameter("attackTime", 1.0, 0.01, 3.0);
    createInternalTriggerParameter("releaseTime", 1.0, 0.1, 10.0);
    createInternalTriggerParameter("pan", 0.0, -1.0, 1.0);
    createInternalTriggerParameter("x", 0.0, -1.0, 1.0);
    createInternalTriggerParameter("y", 0.0, -1.0, 1.0);
    createInternalTriggerParameter("z", 0.0, -10.0, 10.0);
  }

  // The audio processing function
  void onProcess(AudioIOData& io) override {
    // Get the values from the parameters and apply them to the corresponding
    // unit generators. You could place these lines in the onTrigger() function,
    // but placing them here allows for realtime prototyping on a running
    // voice, rather than having to trigger a new voice to hear the changes.
    // Parameters will update values once per audio callback because they
    // are outside the sample processing loop.
    mOsc.freq(getInternalParameterValue("frequency"));
    mAmpEnv.lengths()[0] = getInternalParameterValue("attackTime");
    mAmpEnv.lengths()[2] = getInternalParameterValue("releaseTime");
    mPan.pos(getInternalParameterValue("pan"));
    while (io()) {
      float s1 = mOsc() * mAmpEnv() * getInternalParameterValue("amplitude");
      float s2;
      mEnvFollow(s1);
      mPan(s1, s1, s2);
      io.out(0) += s1;
      io.out(1) += s2;
    }
    // We need to let the synth know that this voice is done
    // by calling the free(). This takes the voice out of the
    // rendering chain
    if (mAmpEnv.done() && (mEnvFollow.value() < 0.001f)) free();
  }

  // The triggering functions just need to tell the envelope to start or release
  // The audio processing function checks when the envelope is done to remove
  // the voice from the processing chain.
  void onTriggerOn() override { mAmpEnv.reset(); }

  void onTriggerOff() override { mAmpEnv.release(); }
};





struct RayBrush : App {

  //variables for graphics
  Material material;
  Light light;
  Mesh mMesh;

  Vec3f pos[N];
  Vec3f offset[N];  // difference from intersection to center of sphere
  float dist[N];    // distance of intersection
  bool hover[N];    // mouse is hovering over sphere
  bool selected[N]; // mouse is down over sphere
  int currentSphereCount; //keep track of how many spheres have been drawn so far


  //variables for sound
  SynthGUIManager<Sound> synthManager{"Sound"};

  void onCreate() override {

    //for graphics
    nav().pos(0, 0, 80); //zoom in and out, higher z is out farther away
    light.pos(0, 0, 80); // not sure what this is
    addSphere(mMesh, 0.5); 
    mMesh.generateNormals();
    currentSphereCount = 1; //initialize current sphere count

    //for sound
    // disable nav control mouse drag to look
    navControl().useMouse(false);



    // Set sampling rate for Gamma objects from app's audio
    gam::sampleRate(audioIO().framesPerSecond());
    imguiInit();
    synthManager.synthRecorder().verbose(true);
  }

  // The audio callback function. Called when audio hardware requires data
  void onSound(AudioIOData& io) override {
    synthManager.render(io);  // Render audio
  }

  void onAnimate(double dt) override {
    // The GUI is prepared here
    imguiBeginFrame();
    // find another way to keep the drawing on the screen, redraw the data color in every frame
    // Draw a window that contains the synth control panel
    synthManager.drawSynthControlPanel();
    imguiEndFrame();
  }


  virtual void onDraw(Graphics &g) override {
    g.clear(0);
    gl::depthTesting(true);
    g.lighting(true);
    // Render the synth's graphics
    synthManager.render(g);
    //draw and color spheres 
    for (int i = 0; i < N; i++) {
      //std::cout<<"in onDraw, i = " << i<< " pos: "<< pos[i] <<std::endl;
      g.pushMatrix();
      g.translate(pos[i]);
      if (selected[i])
        g.color(1, 0, 1);
      else if (hover[i])
        g.color(0, 1, 1);
      else
        g.color(1, 1, 1);
      if(i == 0){
        //color paintbrush blue
        g.color(1, 1, 0);
      }
      g.draw(mMesh);
      g.popMatrix();
    }
    
    // GUI is drawn here
    imguiDraw();

  }

  Vec3d unproject(Vec3d screenPos) {
    auto &g = graphics();
    auto mvp = g.projMatrix() * g.viewMatrix() * g.modelMatrix();
    Matrix4d invprojview = Matrix4d::inverse(mvp);
    Vec4d worldPos4 = invprojview.transform(screenPos);
    return worldPos4.sub<3>(0) / worldPos4.w;
  }

  Rayd getPickRay(int screenX, int screenY) {
    Rayd r;
    Vec3d screenPos;
    screenPos.x = (screenX * 1. / width()) * 2. - 1.;
    screenPos.y = ((height() - screenY) * 1. / height()) * 2. - 1.;
    screenPos.z = -1.;
    Vec3d worldPos = unproject(screenPos);
    r.origin().set(worldPos);

    screenPos.z = 1.;
    worldPos = unproject(screenPos);
    r.direction().set(worldPos);
    r.direction() -= r.origin();
    r.direction().normalize();
    return r;
  }

  bool onMouseMove(const Mouse &m) override {
    // make a ray from mouse location
    Rayd r = getPickRay(m.x(), m.y());

    // intersect ray with each sphere in scene
    for (int i = 0; i < N; i++) {
      // intersect sphere at center pos[i] and radius 0.5f
      // returns the distance of the intersection otherwise -1
      float t = r.intersectSphere(pos[i], 0.5f);
      hover[i] = t > 0.f;
    }
    return true;
  }
  bool onMouseDown(const Mouse &m) override {

    //find current mouse ray position on mouse click
    Rayd r = getPickRay(m.x(), m.y());

    for (int i = 0; i < 1; i++) {
      float t = r.intersectSphere(pos[i], 0.5f);
      selected[i] = t > 0.f;

      // if intersection occured store and offset and distance for moving the
      // sphere
      // also trigger note on
      if (t > 0.f) {
        offset[i] = pos[i] - r(t);
        dist[i] = t;

        // trigger note on
        const float A4 = 220.f;
        note = (m.x()+m.y())/10; 
        std::cout<<"Drawing midi note = "<< note <<std::endl;

        if (note > 0) {
            synthManager.voice()->setInternalParameterValue(
                "frequency", ::pow(2.f, (note - 69.f) / 12.f) * A4);
            synthManager.triggerOn(note);
          }
      }
    }
    return true;
  }
  bool onMouseDrag(const Mouse &m) override {
    Rayd r = getPickRay(m.x(), m.y());
    int i = currentSphereCount;
    // if sphere[0] selected, keep changing sphere[0]s pos and add more spheres at mouse positions
    if (selected[0]) {
      Vec3f newPos = r(dist[0]) + offset[0]; //update get new pos
      pos[0].set(newPos); //update first one
      //make new sphere at pos
      pos[i] = newPos;
      offset[i] = Vec3f();
      dist[i] = 0.f;
      hover[i] = false;
      selected[i] = false;   
    }
    if(currentSphereCount < 1000){
      currentSphereCount++; //added another sphere to view
    }else{
      currentSphereCount= 1;
    }
    
    return true;
  }
  bool onMouseUp(const Mouse &m) override {
    //trigger note off
    synthManager.triggerOff(note);
    // deselect all spheres
    for (int i = 0; i < N; i++)
      selected[i] = false;
  
    return true;
  }
};
int main() {
  RayBrush app;
  app.start();
  return 0;
}
