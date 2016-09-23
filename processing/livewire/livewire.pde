import processing.serial.*;
import geomerative.*;

Serial myPort = null;

RShape wire;
RPoint[] pointPaths;
int extruderStep = 70; //extruder step 1 cm
double benderStep = 19.44; //bender stepper step 1 degree
int benderMinAngle = -108;
int benderMaxAngle = 108;
String benderCenter = "Y1700\n";
String name = "";
int check = 0;
int anothercheck = 0;

void setup() {
  size(600, 600);

  selectInput("Select a file to process:", "fileSelected");
}

void fileSelected(File selection) {
  if (selection == null) {
    println("Window was closed or the user hit cancel.");
    exit();
  } 
  else {
    name = selection.getName();
    println("User selected " + name);
    anothercheck = 1;
  }
}

void draw() {
  if (anothercheck !=0) {
    prepare();
  }
  if (check != 0) {
    makedraw();
  }
}

void prepare() {
  //Serial Library 
  if (Serial.list().length > 0) {
    String portName = Serial.list()[0];
    myPort = new Serial(this, portName, 115200);
    delay(2000);
  }
  myPort.write("Y-4000\n"); //homing
  myPort.write("D500\n");
  RG.init(this);
  RG.ignoreStyles(); 

  //RG.setPolygonizer(RG.UNIFORMLENGTH);
  //RG.setPolygonizerLength(2000);  

  RG.setPolygonizer(RG.ADAPTATIVE);

  wire = RG.loadShape(name);

  pointPaths = wire.getPoints();

  System.out.println(pointPaths.length);

  if (myPort != null) {
    myPort.write("S1\n"); //solenoid on
    myPort.write(benderCenter);//centering bender
  }

  if (pointPaths != null) {
    double[] A = new double[2];
    double[] B = new double[2];
    B[0] = pointPaths[0].x - pointPaths[1].x;
    B[1] = pointPaths[0].y - pointPaths[1].y;
    double offsetAngle = Math.toDegrees(Math.atan2(B[1], -B[0]));
    //System.out.println("Offset Angle " + offsetAngle);

    for (int j = 0; j<pointPaths.length - 3; j++) {
      A[0] = pointPaths[j+1].x - pointPaths[j].x;
      A[1] = pointPaths[j+1].y - pointPaths[j].y;
      B[0] = pointPaths[j+1].x - pointPaths[j+2].x;
      B[1] = pointPaths[j+1].y - pointPaths[j+2].y;

      double magA = Math.sqrt(A[0]*A[0] + A[1]*A[1]);
      double magB = Math.sqrt(B[0]*B[0] + B[1]*B[1]);
      double theta = Math.toDegrees(Math.acos((A[0]*B[0] + A[1]*B[1]) / (magA * magB)));
      double angle = Math.toDegrees(Math.atan2(B[1], -B[0])) - offsetAngle;
      if (angle > 180) {
        angle -= 360;
      }
      offsetAngle += angle;
      if (offsetAngle > 360) {
        offsetAngle -= 360;
      }

      //System.out.println("Nomor " + j);
      //System.out.println(pointPaths[j].x + ", " + pointPaths[j].y);
      //System.out.println("A[0] " + A[0]);
      //System.out.println("A[1] " + A[1]);
      //System.out.println("B[0] " + B[0]);
      //System.out.println("B[1] " + B[1]);
      //System.out.println("MagA " + magA / 1000);
      //System.out.println("MagB " + magB);
      //System.out.println("Theta " + theta);
      System.out.println("Angle " + angle);


      if (angle < benderMinAngle) 
        angle = benderMinAngle;
      else if (angle > benderMaxAngle) 
        angle = benderMaxAngle;

      System.out.print(String.format("X%d\n", (int)(extruderStep*(magA/1000))));

      if (angle >= 0) {
        System.out.print("Y-600\n");
        System.out.print("S0\n");
        System.out.print(String.format("Y%d\n", (int)((benderStep*angle)+200)));
        System.out.print(String.format("Y%d\n", (int)((-benderStep*angle)-200)));
        System.out.print("S1\n");
        System.out.print("Y600\n");
      } else if (angle < 0) {
        System.out.print("Y600\n");
        System.out.print("S0\n");
        System.out.print(String.format("Y%d\n", (int)((benderStep*angle)-200)));
        System.out.print(String.format("Y%d\n", (int)((-benderStep*angle)+200)));
        System.out.print("S1\n");
        System.out.print("Y-600\n");
      }

      //Serial command write
      if (myPort != null) {
        myPort.write(String.format("X%d\n", (int)(extruderStep*(magA/1000))));

        if (angle > 0) {
          myPort.write("Y-600\n");
          myPort.write("S0\n");
          myPort.write(String.format("Y%d\n", (int)((benderStep*angle)+200)));
          myPort.write(String.format("Y%d\n", (int)((-benderStep*angle)-200)));
          myPort.write("S1\n");
          myPort.write("Y600\n");
        }
        if (angle < 0) {
          myPort.write("Y600\n");
          myPort.write("S0\n");
          myPort.write(String.format("Y%d\n", (int)((benderStep*angle)-200)));
          myPort.write(String.format("Y%d\n", (int)((-benderStep*angle)+200)));
          myPort.write("S1\n");
          myPort.write("Y-600\n");
        }
      }
    }
    if (myPort != null) {
      myPort.write("X200\n"); //extrude wire
      myPort.write("Y-1000\n"); //homing bender
      myPort.write("S0\n"); //solenoid off
      myPort.write("D500\n");
    }
  }
  noLoop();
  check = 1;
  anothercheck=0;
}
void makedraw() {
  wire.translate(-wire.getX() + 1000, -wire.getY() + 1000);
  pointPaths = wire.getPoints();

  background(0);
  stroke(255);
  noFill();

  if (pointPaths != null) {
    beginShape();
    for (int j = 0; j<pointPaths.length; j++) {
      pointPaths[j].scale(0.02);
      vertex(pointPaths[j].x, pointPaths[j].y);
      //System.out.println(pointPaths[j].x + ", " + pointPaths[j].y);
    }
    endShape();
  }
  check= 0;
}