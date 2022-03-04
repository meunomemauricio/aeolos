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

// Enclosure Thickness

enc_d_tk = 2.5;
enc_h_tk = 3;

elec_fan_tk = 3; // Between elect. and fan insert

// Electronics Compartment

elec_w = fan_w;
elec_d = 35;
elec_h = 25;

// Insert on the top, for the Fan

enc_fan_tol = 1;  // Tolerance between the Fan and the Enclosure

insert_w = fan_w + enc_fan_tol;
insert_d = fan_d + enc_fan_tol;
insert_h = fan_h + enc_fan_tol;

insert_ofs_h = enc_h_tk + elec_h + elec_fan_tk;

// Honeycomb parameters

hcomb_w = fan_w;
hcomb_d = fan_d * 2;
hcomb_h = fan_h;
hcomb_r = 5;
hcomb_tk = 1;  // Wall Thickness

hcomb_ofs_h = insert_ofs_h;

// Enclosure Dimensions

enc_w = fan_w + 10 * 2;
enc_d = enc_d_tk + elec_d + enc_d_tk;
enc_h = enc_h_tk + elec_h + elec_fan_tk + insert_h + enc_h_tk;
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
 
  difference() {
    cube([width, depth, height]);

    // Honeycomb Pattern
    translate([0, depth + 1, wall_tk/2]) rotate([90, 0, 0]) {
      for (i = [0 : width / h_step ]) {
        for (j = [0 : height / inradius ]) {
          if ((i+j) % 2 == 0) {
            translate([i * h_step, j * inradius, 0])
              hex_cell(depth + 2, radius, wall_tk);
          }
        }
      }
    }
  }
}

/* Enclosure */
module enclosure() {
  difference() {
    translate([0, 0, enc_h/2])
      roundedcube(enc_w, enc_d, enc_h, enc_corner_r);

    // Fan Insert
    translate([-insert_w/2, -insert_d/2, insert_ofs_h])
      cube([insert_w, insert_d, insert_h]);

    // Electronics Compartment
    translate([-elec_w/2, -elec_d/2, enc_h_tk])
      cube([elec_w, elec_d, elec_h]);

    // Mesh
    translate([-hcomb_w/2, -hcomb_d/2, hcomb_ofs_h])
      honeycomb(hcomb_w, hcomb_d, hcomb_h, hcomb_r, hcomb_tk);
  }
}

// Final Assenbly

// translate([0, 0, fan_h/2 + fan_ofs_h])
//   color([.3, .3, .3])
//   fan();

difference() {
  enclosure(); 
  // Transversal Cut
  translate([0, -enc_d/2, enc_h/2])
    cube([enc_w + 20, enc_d, enc_h + 20], center=true);
}