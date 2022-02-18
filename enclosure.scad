// Aeolos Enclosure Design
// Units in mm (millimeters).

/* Cube with Rounded corners. */
module roundedcube(width, depth, height, radius) {
    translate ([-width/2, depth/2, -height/2])
      rotate([90, 0, 0])
      hull() {
        translate([radius, radius, 0])
          cylinder(h=depth, r=radius);

        translate([width - radius, radius, 0])
          cylinder(h=depth, r=radius);

        translate([radius, height - radius, 0])
          cylinder(h=depth, r=radius);

        translate([width - radius, height - radius, 0])
          cylinder(h=depth, r=radius);
      }
}

/* Fan */
module fan() {
    width = 120;  
    depth = 25;  
    height = 120;  
    corner_r = 7.5;

    hole_w = 105;
    hole_d = depth + 2;
    hole_h = 105;
    hole_r = 4.5 / 2;

    duct_d = depth + 2;
    duct_r = 115 / 2;

    difference() {
      roundedcube(width, depth, height, corner_r);
      rotate([90, 0, 0]) {
        // Duct
        translate([0, 0, -duct_d/2])
          cylinder(h=duct_d, r=duct_r);

        // Mount Holes
        translate([-hole_w / 2, -hole_h / 2, -hole_d/2])
          cylinder(h=hole_d, r=hole_r);

        translate([hole_w / 2, -hole_h / 2, -hole_d/2])
          cylinder(h=hole_d, r=hole_r);

        translate([-hole_w / 2, hole_h / 2, -hole_d/2])
          cylinder(h=hole_d, r=hole_r);

        translate([hole_w / 2, hole_h / 2, -hole_d/2])
          cylinder(h=hole_d, r=hole_r);
      }
    }
}

fan();