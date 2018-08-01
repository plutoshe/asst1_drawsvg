#include "viewport.h"

#include "CMU462.h"
#include "matrix3x3.h"

namespace CMU462 {

void ViewportImp::set_viewbox( float x, float y, float span ) {

  // Task 4 (part 2):
  // Set svg to normalized device coordinate transformation. Your input
  // arguments are defined as SVG canvans coordinates.
  Matrix3x3 norm = Matrix3x3::identity();
  norm.zero(0);
  norm[0][0] = norm[1][1] = 1 / (2 * span);
  norm[2][0] = (span - x) / (2 * span);
  norm[2][1] = (span - y) / (2 * span);
  norm[2][2] = 1;

  this->x = x;
  this->y = y;
  this->span = span;

  set_canvas_to_norm(norm);
}
// 190, 140 -> 0, 0
// 200, 150 -> 0.5
// 210, 160 -> 1, 1
// (x - 190) / 20 = 1/20 * x - 190 / 20
// (y - 160) / 20 = 1/20 * y - 160 / 20
// [x 0 0][x]   [ ]
//  0 y 0][y] =
//  0 0 1][1]

void ViewportImp::update_viewbox( float dx, float dy, float scale ) {

  this->x -= dx;
  this->y -= dy;
  this->span *= scale;
  set_viewbox( x, y, span );
}

} // namespace CMU462