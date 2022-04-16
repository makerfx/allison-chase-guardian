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

//Width of platform (in mm)
ringBaseWidth = 7.2;

///////////////////////////////////////////////////////////////





/////EDIT INFO BELOW FOR CUSTOMIZATION/////
/* [Parameters] */
//Overall height of model (in mm)
domeHeight = 80;
//Overall width of model (in mm)
domeWidth = 155;
//Thickness of dome shell (in mm)
domeThickness = 10;

includeRing1 = true;
includeRing2 = true;
includeRing3 = true;
includeRing4 = true;
includeRing5 = true;
includeRing6 = true;
includeRing7 = true;
includeRing8 = true;
includeRing9 = false;





/////////////////////////////////////////////////////////////
////Setup////

// Creating ringsActive vector
ringsAvailable = [ring1, ring2, ring3, ring4, ring5, ring6, ring7, ring8, ring9];

includes = [includeRing1, includeRing2, includeRing3, includeRing4, includeRing5, includeRing6, includeRing7, includeRing8, includeRing9];

ringsActive = [
    for(i = [0:len(ringsAvailable) - 1])
    if(includes[i])
    ringsAvailable[i]
];
    
ringNumber = len(ringsActive);

assert(ringsActive[ringNumber-1]<=domeWidth, "One or more of the rings selected will not fit within the dome");

ringBaseHeight = domeHeight;

difference(){
union(){
    
color("red")
    scale([1, 1, (domeHeight*2)/domeWidth]) sphere(d = domeWidth);
    
color("cyan")
for(i=[0:ringNumber-1])
{
  if (ringsActive[i] == ring1)
  {
    translate([0,0,domeHeight-(ringBaseHeight-1)]) cylinder(h=ringBaseHeight, d=ringsActive[i], center=false);
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
}
color("red")
scale([1, 1, (domeHeight*2)/domeWidth]) sphere(d = domeWidth-(domeThickness*2));
translate([0,0,-domeHeight]) cube([domeWidth,domeWidth,domeHeight*2],center = true);
}

        
