/////IGNORE, SETTING VARIABLES/////

/* [Hidden] */
//Resolution 
$fn = 200;
ring1 = 12;
ring2 = 32;
ring3 = 52;
ring4 = 72;
ring5 = 92;
ring6 = 112;
ring7 = 132;
ring8 = 152;
ring9 = 172;

///////////////////////////////////////////////////////////////





/////EDIT INFO BELOW FOR CUSTOMIZATION/////
/* [Parameters] */
//Height of platform (in mm)
ringBaseHeight = 10;
//Width of platform (in mm)
ringBaseWidth = 7.2;
//Number of rings
ringNumber = 8;
//Overall height of model (in mm)
domeHeight = 80;
//Overall width of model (in mm)
domeWidth = 155;
//Thickness of dome shell (in mm)
domeThickness = 10;
//Specify which rings you would like to use (1=smallest, 9=largest)
ringsActive = [ring1,ring2,ring3,ring4,ring5,ring6,ring7,ring8];





/////////////////////////////////////////////////////////////
////Setup////
assert(len(ringsActive)==ringNumber, "The list of rings you would like to use needs to match the number of rings desired");

assert(ringsActive[ringNumber-1]<=domeWidth, "One or more of the rings selected will not fit within the dome");



color("red")
difference(){
        scale([1, 1, (domeHeight*2)/domeWidth]) sphere(d = domeWidth);
        scale([1, 1, (domeHeight*2)/domeWidth]) sphere(d = domeWidth-(domeThickness*2));
        translate([0,0,-domeHeight]) cube([domeWidth,domeWidth,domeHeight*2],center = true);
}
    

for(i=[0:ringNumber-1])
{
color("cyan")
  if (ringsActive[i] == ring1)
  {
    translate([0,0,domeHeight-(ringBaseHeight-2)]) cylinder(h=ringBaseHeight, d=ringsActive[i], center=false);
  }
  else
  {    
  translate([0,0,(domeHeight*sqrt(pow((domeWidth/2),2)-pow((((ringsActive[i]-ringBaseWidth)/2)),2))/(domeWidth/2))-ringBaseHeight])
    difference(){
        union() {
            cylinder(h=ringBaseHeight, d=ringsActive[i], center=false);
            translate([0,0,ringBaseHeight]) cylinder (h=2, d=       (ringsActive[i])-ringBaseWidth+2, center=false);
        }
        translate([0,0,-.1]) cylinder (h=ringBaseHeight+2.2, d=(ringsActive[i])-ringBaseWidth, center=false);
    }
  }
}

        
