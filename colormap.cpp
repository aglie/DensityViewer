#include "colormap.h"

const Colormap Colormap::BrewerBrownToGreen(vector<int>{0,60,48,1,102,94,53,151,143,128,205,193,199,234,229,245,245,245,246,232,195,223,194,125,191,129,45,140,81,10,84,48,5});
const Colormap Colormap::BrewerGreenToPink(vector<int>{142,1,82,197,27,125,222,119,174,241,182,218,253,224,239,247,247,247,230,245,208,184,225,134,127,188,65,77,146,33,39,100,25});
const Colormap Colormap::BrewerGreenToPurple(vector<int>{64,0,75,118,42,131,153,112,171,194,165,207,231,212,232,247,247,247,217,240,211,166,219,160,90,174,97,27,120,55,0,68,27});
const Colormap Colormap::BrewerOrangeToPurple(vector<int>{45,0,75,84,39,136,128,115,172,178,171,210,216,218,235,247,247,247,254,224,182,253,184,99,224,130,20,179,88,6,127,59,8});
const Colormap Colormap::BrewerRedToBlue(vector<int>{5,48,97,33,102,172,67,147,195,146,197,222,209,229,240,247,247,247,253,219,199,244,165,130,214,96,77,178,24,43,103,0,31});
const Colormap Colormap::BrewerBlackToRed(vector<int>{103,0,31,178,24,43,214,96,77,244,165,130,253,219,199,255,255,255,224,224,224,186,186,186,135,135,135,77,77,77,26,26,26});
const Colormap Colormap::BrewerRedYellowBlue(vector<int>{49,54,149,69,117,180,116,173,209,171,217,233,224,243,248,255,255,191,254,224,144,253,174,97,244,109,67,215,48,39,165,0,38});
const Colormap Colormap::BrewerRedYellowGreen(vector<int>{0,104,55,26,152,80,102,189,99,166,217,106,217,239,139,255,255,191,254,224,139,253,174,97,244,109,67,215,48,39,165,0,38});
const Colormap Colormap::BrewerRedYellowGreenBlue(vector<int>{94,79,162,50,136,189,102,194,165,171,221,164,230,245,152,255,255,191,254,224,139,253,174,97,244,109,67,213,62,79,158,1,66});
//
const map<string,const Colormap& > Colormap::AvailableColormaps = {{"BrownToGreen",BrewerBrownToGreen},
                                                                   //{"GreenToPink",BrewerGreenToPink},
                                                                   //{"GreenToPurple",BrewerGreenToPurple},
                                                                   {"OrangeToPurple",BrewerOrangeToPurple},
                                                                   {"RedToBlue",BrewerRedToBlue},
                                                                   {"BlackToRed",BrewerBlackToRed},
                                                                   {"RedYellowBlue",BrewerRedYellowBlue},
                                                                   {"RedYellowGreen",BrewerRedYellowGreen},
                                                                   {"RedYellowGreenBlue",BrewerRedYellowGreenBlue}};
