int calculate_area(int width, int height);

#define GET_AREA(w, h) calculate_area(w, h)
//                     ^
//                     Cursor here - requesting "goto definition" or completion
