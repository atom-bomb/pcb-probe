#ifndef GCODE_VARIANTS_H
#define GCODE_VARIANTS_H

typedef struct {
  string name ;
  unsigned int first_variable ;
  unsigned int last_variable ;
  string z_probe_result_variable ;
  string pause_command ;
  string probe_command ;
} GCode_Variant ;

const GCode_Variant GCode_Variants[] = {
  { "EMC2", 2000, 5161, "5063", "M60", "G38.2" }, // emc
  { "Mach3", 2100, 5161, "2002", "M0", "G31" },    // mach3
  } ;

typedef enum { emc = 0,
               mach3 } GCode_Variant_Name ;

#endif // GCODE_VARIANTS_H
