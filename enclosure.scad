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

/* Solid Hexagonal Prism. */
module hex(height, radius) {
	cylinder(height, r=radius, $fn=6);
}

/* Hexagom Cell. */
module hex_cell(height, radius, wall_tk) {
	difference() {
		hex(height, radius + wall_tk);
		translate([0, 0, -.1])
			hex(height * 3, radius - wall_tk);
	}
}

/* Honeycomb Mesh.
 *
 * The `radius` parameter is the Circumradius.
 */
module honeycomb(width, depth, height, radius, wall_tk) {
  inradius = cos(30) * radius;
  h_step = 3/2 * radius;  // Horizontal Step
 
  translate([-width/2, 0, -height/2]) difference() {
    // Honeycomb Pattern
    translate([0, depth/2, 0]) rotate([90, 0, 0]) {
      for (i = [0 : width / h_step ]) {
        for (j = [0 : height / inradius ]) {
          if ((i+j) % 2 == 0) {
            translate([i * h_step, j * inradius, 0])
              hex_cell(depth, radius, wall_tk);
          }
        }
      }
    }

    // Trim the Honeycomb excess using 4 cubes
    translate([width / 2, 0, height + radius / 2])
      cube([width * 2, depth * 2, radius], center=true);

    translate([width / 2, 0, -radius / 2])
      cube([width * 2, depth * 2, radius], center=true);

    translate([-(radius + wall_tk) / 2, 0, height / 2])
      cube([radius + wall_tk, depth * 2, height * 2], center=true);

    translate([height + (radius + wall_tk)/2, 0, height / 2])
      cube([radius + wall_tk, depth * 2, height * 2], center=true);
  }
}


/* Fan. */
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

// translate([0, 0, fan_h/2 + 10])
//   fan();

// translate([0, 0, enc_h/2])
//   color([0.5, 0.5, 0.5, .7])
//   enclosure(); 

honeycomb(120, 20, 120, 10, 0.5);