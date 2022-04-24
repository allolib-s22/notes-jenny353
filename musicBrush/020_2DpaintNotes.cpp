#include <cstdio>  // for printing to stdout

#include "Gamma/Analysis.h"
#include "Gamma/Effects.h"
#include "Gamma/Envelope.h"
#include "Gamma/Oscillator.h"

#include "al/app/al_App.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/scene/al_PolySynth.hpp"
#include "al/scene/al_SynthSequencer.hpp"
#include "al/ui/al_ControlGUI.hpp"
#include "al/ui/al_Parameter.hpp"
#include "al/io/al_Window.hpp"
#include "al/types/al_Color.hpp"
//#include "al/io/al_Graphics.hpp"
#include <vector>


// using namespace gam;
using namespace al;

// Paints single note (midi 72) on draw and color to match that frequency
// all notes stop on mouse up 


int x_pos;
int y_pos;
int note = 72;
//std::vector<int> notes; 

class Paint : public SynthVoice {
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

  // The graphics processing function
  void onProcess(Graphics& g) override {
    // Get the paramter values on every video frame, to apply changes to the
    // current instance
    float frequency = getInternalParameterValue("frequency");
    float amplitude = getInternalParameterValue("amplitude");

    // Now draw
    g.pushMatrix();
    std::cout<< "freq: "<< frequency << std::endl;
    std::cout<< "amp: "<< amplitude << std::endl;
    std::cout<<"x: "<< x_pos<< " , y: " << y_pos<<std::endl;  
    //g.translate(frequency / 200 - 3, amplitude, -8);
    //std::cout<<frequency << " , " <<amplitude<<std::endl;  
    g.translate((x_pos/200.0)-1.5, (y_pos/-300.0)+.8, -3.8);  //x, y (-.5 bottom to +.5 top), z values 
    //g.translate(getInternalParameterValue("x") , getInternalParameterValue("y"), getInternalParameterValue("z"));  //x, y (-.5 bottom to +.5 top), z values 
    //g.scale(1 - amplitude, amplitude, 1);
    g.color(mEnvFollow.value(), frequency / 1000, mEnvFollow.value() * 10, 0.4);
    g.draw(mMesh);
    g.popMatrix();
  }

  // The triggering functions just need to tell the envelope to start or release
  // The audio processing function checks when the envelope is done to remove
  // the voice from the processing chain.
  void onTriggerOn() override { mAmpEnv.reset(); }

  void onTriggerOff() override { mAmpEnv.release(); }
};

// We make an app.
class MyApp : public App {
 public:
  // GUI manager for Paint voices
  // The name provided determines the name of the directory
  // where the presets and sequences are stored
  SynthGUIManager<Paint> synthManager{"Paint"};

  // This function is called right after the window is created
  // It provides a grphics context to initialize ParameterGUI
  // It's also a good place to put things that should
  // happen once at startup.
  void onCreate() override {
    navControl().active(false);  // Disable navigation via keyboard, since we
                                 // will be using keyboard for note triggering

    // Set sampling rate for Gamma objects from app's audio
    gam::sampleRate(audioIO().framesPerSecond());

    imguiInit();

    // Play example sequence. Comment this line to start from scratch
    //synthManager.synthSequencer().playSequence("synth1.synthSequence");
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

  // The graphics callback function.
  void onDraw(Graphics& g) override {
    g.clear(); 
    //g.color(200.0,255.0,255.0, 1.f);
    //g.camera(Viewpoint::IDENTITY);  // Ortho [-1:1] x [-1:1]
    g.camera(Viewpoint::ORTHO_FOR_2D);  // Ortho [0:width] x [0:height]
    // Render the synth's graphics
    synthManager.render(g);
    //g.color(255.0,255.0,255.0, 1.f);
    // GUI is drawn here
    imguiDraw();
  }


 	

  // Whenever a key is pressed, this function is called
  bool onKeyDown(Keyboard const& k) override {
    // const float A4 = 432.f;
    const float A4 = 440.f;
    if (ParameterGUI::usingKeyboard()) {  // Ignore keys if GUI is using
                                          // keyboard
      return true;
    }
    if (k.shift()) {
      // If shift pressed then keyboard sets preset
      int presetNumber = asciiToIndex(k.key());
      synthManager.recallPreset(presetNumber);
    } 
    // else if (k.BACKSPACE()) {
    //   //clear screen
    
    // }
      else {
      // Otherwise trigger note for polyphonic synth
      int midiNote = asciiToMIDI(k.key());
      std::cout<< "Midinote: "<< midiNote <<std::endl;
      if (midiNote > 0) {
        synthManager.voice()->setInternalParameterValue(
            "frequency", ::pow(2.f, (midiNote - 69.f) / 12.f) * A4);
        synthManager.triggerOn(midiNote);
      }
    }
    return true;
  }

  bool onMouseDown(Mouse const & m) override {
    std::cout<< "onMouseDown" <<std::endl;
    
   // notes.push_back(note) ;
    const float A4 = 220.f;
    note = m.x();
    if (note > 0) {
        synthManager.voice()->setInternalParameterValue(
            "frequency", ::pow(2.f, (note - 69.f) / 12.f) * A4);
        synthManager.triggerOn(note);
      }
    std::cout<<"note: "<< note << std::endl;
    x_pos = m.x();
    y_pos = m.y();
    return true;
  };


  bool onMouseUp(Mouse const & m) override{
    std::cout<< "onMouseUp" <<std::endl;
    synthManager.triggerOff(note);
    //synthManager.synth().allNotesOff();
    
      
    return true;

  };
  //draw notes, when gesture is done, then make sound 

  bool onMouseDrag(Mouse const & m) override {
    std::cout<< "onMouseDrag" <<std::endl;
    // int note =  72;
    // //notes.push_back(note) ;
    // const float A4 = 220.f;
    // if (note > 0) {
    //     synthManager.voice()->setInternalParameterValue(
    //         "frequency", ::pow(2.f, (note - 69.f) / 12.f) * A4);
    //     synthManager.triggerOn(note);
    //   }
    // std::cout<<"note: "<< note <<std::endl;
    x_pos = m.x();
    y_pos = m.y();
    return true;
  };

  // Whenever a key is released this function is called
  bool onKeyUp(Keyboard const& k) override {
    int midiNote = asciiToMIDI(k.key());
    if (midiNote > 0) {
      synthManager.triggerOff(midiNote);
    }
    return true;
  }


  void onExit() override { imguiShutdown(); }
};

int main() {
  // Create app instance
  MyApp app;

  // Set up audio
  app.configureAudio(48000., 512, 2, 0);

  app.start();
  
  return 0;
}





