// Walkman Case Design
// Units: mm

// --- Parameters ---
width = 80;
height = 120;
depth = 25;
wall_thickness = 2;
corner_radius = 5;

// Colors for preview
color_case = [0.9, 0.9, 0.95, 0.3]; // Transparent whitish
color_tape = [0.2, 0.2, 0.2, 1];
color_button_play = [1, 0.4, 0, 1]; // Orange
color_button_stop = [0.1, 0.1, 0.1, 1]; // Black
color_details = [0.6, 0.6, 0.6, 1];

$fn = 50; // Smoothness

module rounded_cube(size, radius) {
    if (radius == 0) {
        cube(size);
    } else {
        hull() {
            translate([radius, radius, 0]) cylinder(h=size[2], r=radius);
            translate([size[0]-radius, radius, 0]) cylinder(h=size[2], r=radius);
            translate([size[0]-radius, size[1]-radius, 0]) cylinder(h=size[2], r=radius);
            translate([radius, size[1]-radius, 0]) cylinder(h=size[2], r=radius);
        }
    }
}

module case_shell() {
    color(color_case) 
    difference() {
        rounded_cube([width, height, depth], corner_radius);
        
        // Hollow inside
        translate([wall_thickness, wall_thickness, wall_thickness])
            rounded_cube([width - 2*wall_thickness, height - 2*wall_thickness, depth - 2*wall_thickness], corner_radius - wall_thickness);
        
        // Tape Window Cutout
        translate([15, 40, depth - 5])
            cube([width - 30, height - 70, 10]);
            
        // Port Cutouts (Side)
        translate([width - 5, 30, depth/2])
            rotate([0, 90, 0])
            cylinder(h=10, r=4); // Headphone jack
            
        translate([width - 5, 50, depth/2 - 3])
             cube([10, 5, 6]); // USB-C / Charging port
    }
}

module tape_mechanism() {
    // Decorative tape reels
    translate([width/2 - 14, height/2 - 10, depth/2]) {
        color([0.3, 0.3, 0.3]) cylinder(h=4, r=10);
        translate([0,0,4]) color([1,1,1]) cylinder(h=1, r=3);
        
        // Tape shape
        translate([0, -12, 0])
             difference() {
                 cube([28, 24, 2], center=true);
             }
    }
    
    translate([width/2 + 14, height/2 - 10, depth/2]) {
        color([0.3, 0.3, 0.3]) cylinder(h=4, r=10);
        translate([0,0,4]) color([1,1,1]) cylinder(h=1, r=3);
    }
    
    // Tape window glass
    color([0.1, 0.1, 0.1, 0.5])
    translate([15, 40, depth - wall_thickness])
        cube([width - 30, height - 70, wall_thickness]);
}

module controls() {
    // Main Buttons on top right side
    button_y = height - 20;
    
    // Orange Play Button
    translate([width + 2, button_y, depth - 10])
    rotate([0, 90, 0]) {
        color(color_button_play)
        union() {
            cylinder(h=6, r=6);
            translate([0,0,6]) sphere(r=6);
        }
    }
    
    // Black Stop Button
    translate([width + 2, button_y - 18, depth - 10])
    rotate([0, 90, 0]) {
        color(color_button_stop)
        union() {
            cylinder(h=6, r=5);
            translate([0,0,6]) sphere(r=5);
        }
    }
    
    // Side Switches
    translate([width, 15, 10])
        color([0.8, 0.8, 0.8])
        cube([3, 10, 4]);
}

module internals_mockup() {
    // PCB / Tech details visible through transparency
    translate([5, 5, 5]) 
        color([0, 0.4, 0])
        cube([width-10, height-10, 2]);
        
    // Battery
    translate([10, 10, 8])
        color([0.2, 0.2, 0.2])
        cube([20, 40, 5]);
}

// --- Assembly ---

// Use this to check intersection
// intersection() { ... }

union() {
    case_shell();
    tape_mechanism();
    controls();
    internals_mockup();
}

// To export for printing, you might want to render just the shell:
// case_shell();
