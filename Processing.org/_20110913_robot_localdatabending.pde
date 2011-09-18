/**
 * control ussbsc32 servos via OSC messages
 * for databending.net
 * thx everyone for the examples (controlP5, osc, serial) 
 */

final String version = "15-09-2011";
 
import netP5.*;         
import oscP5.*;
import controlP5.*;
import java.net.InetAddress;
import processing.serial.*;

// Network stuff
InetAddress inet;
String myIP;
OscP5 oscP5,oscP5_send;
OscMessage myOscMessage;
NetAddress myBroadcastLocation;
//Incoming port te listen to
int port =7000; 

// variables to deal with the incoming data
HashMap   oscaddressesHash;
ArrayList oscaddressesList;
int oscaddressCounter = 0;

// gfx
ControlP5 controlP5;  
Textlabel title, footer, ipTextlabel;
ListBox list1;
CheckBox checkbox;

// COM servo variables
Serial myPort;

// global stuff
boolean go; // on off switch for COMmunication
Timer timer,timerOSC,timerAdv;
int oscCount;

//pair
int lastServo,totalServo;
String lastOSC;
boolean isServo,isOSC;
String[] pairs; //needed to learn more processing :) // needed to keep SERVO -> "OSC" pairs
HashMap   pairHash; // needed to keep "OSC" key for servo pairs
ArrayList pairList; // not yet used

//servo values //Todo buffer
int[] servoValues; //needed to keep updated paired values

void setup() {
 
  go = false;
  totalServo=6;
  
  size(885, 630); // remove OPENGL to run in normal mode 
  frameRate(25);

  
 // doing all the prone to error stuff like COM and network 
 try {
    inet = InetAddress.getLocalHost();
    myIP = inet.getHostAddress();
  }
  catch (Exception e) {
    //e.printStackTrace();
    println("Can't get no sleep (IP)");
  } 
 try {
      String portName = Serial.list()[1]; //TODO get list dinamically
      println("Com port: " + portName);
      myPort = new Serial(this, portName, 115200);  }
  catch (Exception e) {
      //e.printStackTrace();
      println("Can't connect to COM");
  }
    oscP5 = new OscP5(this, 7000);
//  oscP5_send = new OscP5(this,port);
//  myBroadcastLocation = new NetAddress("10.0.0.120",8000);
//  myOscMessage = new OscMessage("/puppet/out/inputs");
//  myOscMessage.add("handleft,handright,knees,shoulders,tilt");
//  oscP5_send.send(myOscMessage, myBroadcastLocation);
  

  oscaddressesHash         = new HashMap();
  oscaddressesList         = new ArrayList();

  noStroke();
  fill(32);
  rect(0, 0, width, 40);
  fill(96);
  rect(0, 590, width, 600);
  fill(0);
  
  
  controlP5 = new ControlP5(this);
  title = controlP5.addTextlabel("title", "USBSSC32 OSC String Puppet Controller", 25, 10);
  ipTextlabel   = controlP5.addTextlabel("ipTextlabel","IP address: "+myIP+" ",25,27);
  
  //adding all servos as p5 sliders
  for(int i = 0; i < totalServo; i ++){ 
    String servoName = "s"+i;
    controlP5.addSlider(servoName,500,2500,50,40,50 + 25*i,100,20).setId(i);
  }
  // adding a checkbox to select manual
  checkbox = controlP5.addCheckBox("checkBox",12,55);  
  // make adjustments to the layout of a checkbox.
  checkbox.setColorForeground(color(120));
  checkbox.setColorActive(color(255));
  checkbox.setColorLabel(color(128));
  checkbox.setItemsPerRow(1);
  checkbox.setSpacingColumn(50);
  checkbox.setSpacingRow(14);
  // add items to a checkbox.
  for(int i = 0; i < totalServo; i ++){
  checkbox.addItem("m"+i,i);
  }
  
  list1 = controlP5.addListBox("list1",180,80,300,200);
  list1.setLabel("received OSC addresses:");
  
  controlP5.addButton("GO",0,25,400,49,20);
  controlP5.addButton("STOP",0,76,400,49,20);

  controlP5.addSlider("OSCperminute",0,50,50,180,50,300,10);
    controlP5.controller("OSCperminute").setBehavior(new ControlBehavior() {
    float a = 0;
    public void update() { setValue(oscCount); }
  });
  
  footer = controlP5.addTextlabel("footer", " Servo controller via OSC by Radu Daniel for databending (www.databending.net)", 25, 605);
 
  // timer for updating Servos 
  timer = new Timer(10);
  timer.start();
  
  // timer for showing OSC updates
  timerOSC = new Timer(1000);
  timerOSC.start();
  
  //timer advertize
  timerAdv = new Timer(5000);
  timerAdv.start();
  
  pairs = new String[10]; //goddamn you strings, you are crazy when not allocated!
  servoValues = new int[10]; //goddamn you strings, you are crazy when not allocated!
  pairHash         = new HashMap();
  pairList         = new ArrayList();  //not yet used
}

void draw() {
  
  //use a timer to check how many new messages have been recived by OSC (mind the class)
  if (timerOSC.isFinished())
  {
    oscCount=0;
    timerOSC.start();
    
    
  }
  
//  if (timerAdv.isFinished())
//  {
//  
//    myOscMessage = new OscMessage("/puppet/out/inputs");
//    myOscMessage.add("handleft,handright,knees,shoulders,tilt");
//    oscP5_send.send(myOscMessage, myBroadcastLocation);
//    timerAdv.start();
//  }
  
  if (go) {
    
    if (timer.isFinished()) {
      
      String comString = "";
      //loop through servovalues
      for (int servoNr = 0; servoNr < totalServo; servoNr++ ){
        if ((int)checkbox.arrayValue()[servoNr] != 1) { // if not manual
          int remapServo = servoNr;        
          if ( servoNr > 0 ) comString = comString + " ";
          comString = comString + "#" + remapServo +" P" + servoValues[servoNr];        
        }
      }
      
      println (comString);  
      myPort.write(comString +"\r\n");    
      timer.start();
         
   }
 }
}


public void GO () {  
  go = true;
}

public void STOP () {  
  go = false;
}

 
public void controlEvent(ControlEvent theEvent) {

  if (theEvent.isGroup()) {
    int value = int(theEvent.group().value());
    
   // println("got an event from "+theEvent.group().name()+"\t");
   
      if(theEvent.group().name() == "checkbox")
      {
        
      }
   
      if(theEvent.group().name() == "list1")
      { 
        String s = (String) oscaddressesList.get(value);
        println ("OSC: " + s);
        lastOSC = s;
        if ( isServo ) pair (lastServo,lastOSC);
        else isOSC = true;
      }
  } else {
          //&& ((int)checkbox.arrayValue()[theEvent.controller().id()] == 1)
         if ((theEvent.controller().id() >= 0 ) && (theEvent.controller().id() < 10) ) { // coming from servo sliders AND ONLY IF ITs MANUAL        
         
         if ((int)checkbox.arrayValue()[theEvent.controller().id()] == 1) {
         println("Servo: "+theEvent.controller().id() + " with manual " + (int)checkbox.arrayValue()[theEvent.controller().id()] );
         
         lastServo = theEvent.controller().id();
         if (checkbox.getState(theEvent.controller().id())) {
           if (pairHash.containsKey(pairs[theEvent.controller().id()]))  { // already paired
             pairHash.remove(pairs[theEvent.controller().id()]);
             pairs[theEvent.controller().id()] = null;
   
             println("Pair removed");
            
           }else if ( isOSC ) pair(lastServo,lastOSC);
           else isServo = true;
         }
         }
         
         }
  }
  
}

void oscEvent(OscMessage oscMessage) {
  
  oscCount++;

  if (!oscaddressesHash.containsKey(oscMessage.addrPattern())) { //new address
      oscaddressesHash.put(oscMessage.addrPattern(),new Integer(0));
      
       list1.addItem(oscMessage.addrPattern(),oscaddressCounter);
       oscaddressesList.add(new String(oscMessage.addrPattern())); // index will be the same as the oscaddressCounter
       oscaddressCounter++;   
  }
  
  // for each pair
   if(pairHash.containsKey(oscMessage.addrPattern()))
      { 

          String ssosc = oscMessage.addrPattern();
          
          Integer sservo = (Integer) pairHash.get(oscMessage.addrPattern());
          
          int vvalue;
          if (sservo == 1){ //tilt
              vvalue = int(map(oscMessage.get(0).floatValue()*100,0,99,1500,1600)); //mega rough conversion to engine talk
             } else if (sservo == 0){ //shoulders
              vvalue = int(map(oscMessage.get(0).floatValue()*100,0,99,1700,1300)); //mega rough conversion to engine talk
             } else if (sservo == 2){ //right hand
              vvalue = int(map(oscMessage.get(0).floatValue()*100,0,99,1484,625)); //mega rough conversion to engine talk
             } else if (sservo == 3){ //left hand
              vvalue = int(map(oscMessage.get(0).floatValue()*100,0,99,1589,578)); //mega rough conversion to engine talk
             } else {
          vvalue = int(map(oscMessage.get(0).floatValue()*100,0,99,500,2500)); //mega rough conversion to engine talk
            }
          
        // println (ssosc + " is paired " + " to servo " + sservo + " which just recived " + vvalue);
        //println(oscMessage.addrPattern() + " is paired to " + pairHash.get(oscMessage.addrPattern()) + " -- " + sservo);
        
         servoValues[sservo] = vvalue; //add it to the servo values
         //update slider somehow
         controlP5.controller("s"+sservo).setValue(vvalue);
      }  
  
  print("received!");
  print(" "+oscMessage.addrPattern());
  String typeTag = oscMessage.typetag();
  print(" "+typeTag);
  print(" args:");
  
  for(int i=0 ; i<typeTag.length() ; i++){

   switch( typeTag.charAt(i) ){
     
    case 'i': print( oscMessage.get(i).intValue() );
      break;

    case 'f': print( oscMessage.get(i).floatValue() );
      break;
 
    case 's': print( oscMessage.get(i).stringValue() );
      break;     
     
   }
  
    print(" ");
  }
  println("");
  
}

void pair (int i, String s) {
 
  
  
  
  if(!pairHash.containsKey(s)) {
    pairHash.put(new String(s),new Integer(i));
     println("Pairing " + i + " " + s);
     checkbox.toggle(i);
     pairs[i] = s;
  }
  else {
     println("Pairing " + i + " " + s + " failed - OSC already paired"); //todo 1:n
  }
  
  isServo = false;
  isOSC = false;
}

