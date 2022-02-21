// Aeolos Enclosure Design
// Units in mm (millimeters).

// Fan Dimensions

fan_w = 120;  
fan_d = 25;  
fan_h = 120;  
fan_corner_r = 7.5;

fan_hole_w = 105;
fan_hole_d = fan_d + 2;
fan_hole_h = 105;
fan_hole_r = 4.5 / 2;

fan_duct_d = fan_d + 2;
fan_duct_r = 115 / 2;


// Tolerance between the Fan and the Enclosure
enc_fan_tol = 1;

// Enclosure Dimensions

enc_w = fan_w + 5 * 2;
enc_d = fan_d + 2.5 * 2;
enc_h = fan_h + 5 * 2;
enc_corner_r = 10;

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
  difference() {
    roundedcube(fan_w, fan_d, fan_h, fan_corner_r);
    rotate([90, 0, 0]) {
      // Duct
      translate([0, 0, -fan_duct_d/2])
        cylinder(h=fan_duct_d, r=fan_duct_r);

      // Mount fan_holes
      translate([-fan_hole_w / 2, -fan_hole_h / 2, -fan_hole_d/2])
        cylinder(h=fan_hole_d, r=fan_hole_r);

      translate([fan_hole_w / 2, -fan_hole_h / 2, -fan_hole_d/2])
        cylinder(h=fan_hole_d, r=fan_hole_r);

      translate([-fan_hole_w / 2, fan_hole_h / 2, -fan_hole_d/2])
        cylinder(h=fan_hole_d, r=fan_hole_r);

      translate([fan_hole_w / 2, fan_hole_h / 2, -fan_hole_d/2])
        cylinder(h=fan_hole_d, r=fan_hole_r);
    }
  }
}

/* Enclosure */
module enclosure() {
  difference() {
    roundedcube(enc_w, enc_d, enc_h, enc_corner_r);

    insert_w = fan_w + enc_fan_tol;
    insert_d = fan_d + enc_fan_tol;
    insert_h = fan_h + enc_fan_tol + 5;

    translate([0, 0, 5])
      cube([insert_w, insert_d, insert_h], center=true);
  }
}

// Final Assenbly

translate([0, 0, fan_h/2 + 10])
  fan();

translate([0, 0, enc_h/2])
  color([0.5, 0.5, 0.5, .7])
  enclosure(); 