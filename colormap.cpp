#include "colormap.h"

const Colormap Colormap::BrewerGreenToBrown(vector<int>{84,48,5,140,81,10,191,129,45,223,194,125,246,232,195,245,245,245,199,234,229,128,205,193,53,151,143,1,102,94,0,60,48});
const Colormap Colormap::BrewerGreenToPink(vector<int>{142,1,82,197,27,125,222,119,174,241,182,218,253,224,239,247,247,247,230,245,208,184,225,134,127,188,65,77,146,33,39,100,25});
const Colormap Colormap::BrewerGreenToPurple(vector<int>{64,0,75,118,42,131,153,112,171,194,165,207,231,212,232,247,247,247,217,240,211,166,219,160,90,174,97,27,120,55,0,68,27});
const Colormap Colormap::BrewerPurpleToOrange(vector<int>{127,59,8,179,88,6,224,130,20,253,184,99,254,224,182,247,247,247,216,218,235,178,171,210,128,115,172,84,39,136,45,0,75});
const Colormap Colormap::BrewerBlueToRed(vector<int>{103,0,31,178,24,43,214,96,77,244,165,130,253,219,199,247,247,247,209,229,240,146,197,222,67,147,195,33,102,172,5,48,97});
const Colormap Colormap::BrewerBlackToRed(vector<int>{103,0,31,178,24,43,214,96,77,244,165,130,253,219,199,255,255,255,224,224,224,186,186,186,135,135,135,77,77,77,26,26,26});
const Colormap Colormap::BrewerBlueYellowRed(vector<int>{165,0,38,215,48,39,244,109,67,253,174,97,254,224,144,255,255,191,224,243,248,171,217,233,116,173,209,69,117,180,49,54,149});
const Colormap Colormap::BrewerGreenYellowRed(vector<int>{165,0,38,215,48,39,244,109,67,253,174,97,254,224,139,255,255,191,217,239,139,166,217,106,102,189,99,26,152,80,0,104,55});
const Colormap Colormap::BrewerBlueGreenYellowRed(vector<int>{158,1,66,213,62,79,244,109,67,253,174,97,254,224,139,255,255,191,230,245,152,171,221,164,102,194,165,50,136,189,94,79,162});

const map<string,const Colormap& > Colormap::AvailableColormaps = {{"GreenToBrown",BrewerGreenToBrown},
                                                                   {"GreenToPink",BrewerGreenToPink},
                                                                   {"GreenToPurple",BrewerGreenToPurple},
                                                                   {"PurpleToOrange",BrewerPurpleToOrange},
                                                                   {"BlueToRed",BrewerBlueToRed},
                                                                   {"BlackToRed",BrewerBlackToRed},
                                                                   {"BlueYellowRed",BrewerBlueYellowRed},
                                                                   {"GreenYellowRed",BrewerGreenYellowRed},
                                                                   {"BlueGreenYellowRed",BrewerBlueGreenYellowRed}};
