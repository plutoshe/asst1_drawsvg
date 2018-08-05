#include "software_renderer.h"

#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>

#include "triangulation.h"

using namespace std;

namespace CMU462 {


// Implements SoftwareRenderer //

void SoftwareRendererImp::draw_svg( SVG& svg ) {
  if (this->super_sample_buffer != NULL) {
    free(this->super_sample_buffer);
  }
  this->super_sample_buffer = this->create_supersampling_buf();
  // set top level transformation
  transformation = canvas_to_screen;

  // draw all elements
  for ( size_t i = 0; i < svg.elements.size(); ++i ) {
    draw_element(svg.elements[i]);
  }

  // draw canvas outline
  Vector2D a = transform(Vector2D(    0    ,     0    )); a.x--; a.y++;
  Vector2D b = transform(Vector2D(svg.width,     0    )); b.x++; b.y++;
  Vector2D c = transform(Vector2D(    0    ,svg.height)); c.x--; c.y--;
  Vector2D d = transform(Vector2D(svg.width,svg.height)); d.x++; d.y--;

  rasterize_line(a.x, a.y, b.x, b.y, Color::Black);
  rasterize_line(a.x, a.y, c.x, c.y, Color::Black);
  rasterize_line(d.x, d.y, b.x, b.y, Color::Black);
  rasterize_line(d.x, d.y, c.x, c.y, Color::Black);

  // resolve and send to render target
  resolve();

}

void SoftwareRendererImp::set_sample_rate( size_t sample_rate ) {

  // Task 3:
  // You may want to modify this for supersampling support
  if (sample_rate < 1) {
    sample_rate = 1;
  }
  this->sample_rate = sample_rate;
  if (this->super_sample_buffer != NULL) {
    free(this->super_sample_buffer);
  }
  this->super_sample_buffer = this->create_supersampling_buf();

}

void SoftwareRendererImp::set_render_target( unsigned char* render_target,
                                             size_t width, size_t height ) {

  // Task 3:
  // You may want to modify this for supersampling support
  this->render_target = render_target;
  this->target_w = width;
  this->target_h = height;
  if (this->super_sample_buffer != NULL) {
    free(this->super_sample_buffer);
  }
  this->super_sample_buffer = this->create_supersampling_buf();
}

unsigned char* SoftwareRendererImp::create_supersampling_buf() {
  int num_samples = this->sample_rate * this->sample_rate;

  size_t buf_size = num_samples * 4 * this->target_h * this->target_w;
  cout << "Buf size: " << buf_size << "\n";
  unsigned char* buf = (unsigned char *) malloc(buf_size);

  if (buf == NULL) {
    cout << "malloc failed\n";
    exit(1);
  }

  memset(buf, 255, buf_size);
  return buf;
}

void SoftwareRendererImp::draw_element( SVGElement* element ) {

  // Task 4 (part 1):
  // Modify this to implement the transformation stack
  Matrix3x3 oldTransform = transformation;
  transformation = transformation * element->transform;
  switch(element->type) {
    case POINT:
      draw_point(static_cast<Point&>(*element));
      break;
    case LINE:
      draw_line(static_cast<Line&>(*element));
      break;
    case POLYLINE:
      draw_polyline(static_cast<Polyline&>(*element));
      break;
    case RECT:
      draw_rect(static_cast<Rect&>(*element));
      break;
    case POLYGON:
      draw_polygon(static_cast<Polygon&>(*element));
      break;
    case ELLIPSE:
      draw_ellipse(static_cast<Ellipse&>(*element));
      break;
    case IMAGE:
      draw_image(static_cast<Image&>(*element));
      break;
    case GROUP:
      draw_group(static_cast<Group&>(*element));
      break;
    default:
      break;
  }
  transformation = oldTransform;

}


// Primitive Drawing //

void SoftwareRendererImp::draw_point( Point& point ) {

  Vector2D p = transform(point.position);
  rasterize_point( p.x, p.y, point.style.fillColor );

}

void SoftwareRendererImp::draw_line( Line& line ) {

  Vector2D p0 = transform(line.from);
  Vector2D p1 = transform(line.to);
  rasterize_line( p0.x, p0.y, p1.x, p1.y, line.style.strokeColor );

}

void SoftwareRendererImp::draw_polyline( Polyline& polyline ) {

  Color c = polyline.style.strokeColor;

  if( c.a != 0 ) {
    int nPoints = polyline.points.size();
    for( int i = 0; i < nPoints - 1; i++ ) {
      Vector2D p0 = transform(polyline.points[(i+0) % nPoints]);
      Vector2D p1 = transform(polyline.points[(i+1) % nPoints]);
      rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    }
  }
}

void SoftwareRendererImp::draw_rect( Rect& rect ) {

  Color c;

  // draw as two triangles
  float x = rect.position.x;
  float y = rect.position.y;
  float w = rect.dimension.x;
  float h = rect.dimension.y;

  Vector2D p0 = transform(Vector2D(   x   ,   y   ));
  Vector2D p1 = transform(Vector2D( x + w ,   y   ));
  Vector2D p2 = transform(Vector2D(   x   , y + h ));
  Vector2D p3 = transform(Vector2D( x + w , y + h ));

  // draw fill
  c = rect.style.fillColor;
  if (c.a != 0 ) {
    rasterize_triangle( p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c );
    rasterize_triangle( p2.x, p2.y, p1.x, p1.y, p3.x, p3.y, c );
  }

  // draw outline
  c = rect.style.strokeColor;
  if( c.a != 0 ) {
    rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    rasterize_line( p1.x, p1.y, p3.x, p3.y, c );
    rasterize_line( p3.x, p3.y, p2.x, p2.y, c );
    rasterize_line( p2.x, p2.y, p0.x, p0.y, c );
  }

}

void SoftwareRendererImp::draw_polygon( Polygon& polygon ) {

  Color c;

  // draw fill
  c = polygon.style.fillColor;
  if( c.a != 0 ) {

    // triangulate
    vector<Vector2D> triangles;
    triangulate( polygon, triangles );

    // draw as triangles
    for (size_t i = 0; i < triangles.size(); i += 3) {
      Vector2D p0 = transform(triangles[i + 0]);
      Vector2D p1 = transform(triangles[i + 1]);
      Vector2D p2 = transform(triangles[i + 2]);
      rasterize_triangle( p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c );
    }
  }

  // draw outline
  c = polygon.style.strokeColor;
  if( c.a != 0 ) {
    int nPoints = polygon.points.size();
    for( int i = 0; i < nPoints; i++ ) {
      Vector2D p0 = transform(polygon.points[(i+0) % nPoints]);
      Vector2D p1 = transform(polygon.points[(i+1) % nPoints]);
      rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    }
  }
}

void SoftwareRendererImp::draw_ellipse( Ellipse& ellipse ) {

  // Extra credit

}

void SoftwareRendererImp::draw_image( Image& image ) {

  Vector2D p0 = transform(image.position);
  Vector2D p1 = transform(image.position + image.dimension);

  rasterize_image( p0.x, p0.y, p1.x, p1.y, image.tex );
}

void SoftwareRendererImp::draw_group( Group& group ) {

  for ( size_t i = 0; i < group.elements.size(); ++i ) {
    draw_element(group.elements[i]);
  }

}

// Rasterization //

// The input arguments in the rasterization functions
// below are all defined in screen space coordinates

void SoftwareRendererImp::rasterize_point( float x, float y, Color color ) {

  // fill in the nearest pixel
  int sx = (int) floor(x);
  int sy = (int) floor(y);

  // check bounds
  if ( sx < 0 || sx >= target_w ) return;
  if ( sy < 0 || sy >= target_h ) return;
  sx *= sample_rate;
  sy *= sample_rate;
  // fill sample - NOT doing alpha blending!
  for (int i = 0; i < sample_rate; i++)
    for (int j = 0; j < sample_rate; j++)
      set_sample_buf(sx+i, sy+j, color);

}

void SoftwareRendererImp::set_sample_buf(int x, int y, Color color) {

  // fill in the nearest pixel

  // check bounds
  if ( x < 0 || x >= target_w * sample_rate) return;
  if ( y < 0 || y >= target_h * sample_rate) return;
  super_sample_buffer[4 * (x + y * target_w * sample_rate)] = (uint8_t) (color.r * 255);
  super_sample_buffer[4 * (x + y * target_w * sample_rate) + 1] = (uint8_t) (color.g * 255);
  super_sample_buffer[4 * (x + y * target_w * sample_rate) + 2] = (uint8_t) (color.b * 255);
  super_sample_buffer[4 * (x + y * target_w * sample_rate) + 3] = (uint8_t) (color.a * 255);
}



Color convertColor(Color origin, float brightness) {
  origin = Color::White * (1-brightness) + origin * brightness;

  return origin;
}

float fPart(float x) {
  return x - floor(x);
}

float rfPart(float x) {
  return 1 - fPart(x);
}

void SoftwareRendererImp::rasterize_line( float x0, float y0,
                                          float x1, float y1,
                                          Color color) {
  // Task 1:
  // Implement line rasterization
    // Implement line rasterization
  x0 *= sample_rate;
  y0 *= sample_rate;
  x1 *= sample_rate;
  y1 *= sample_rate;
  bool switchXYaxis = false;
  // printf("%.3f %.3f %.3f %.3f\n", x0, y0, x1, y1);
  if (abs(y0 - y1) > abs(x0 - x1)) {
    //drawline by y axis.(for y0 to y1)
    swap(x0, y0);
    swap(x1, y1);
    switchXYaxis = true;
  }
    //drawline by x axis.(for x0 to x1)
  if (x1 < x0) {
    swap(y0, y1);
    swap(x0, x1);
  }

  // printf("convert to: %d %d %d %d\n", sx0, sy0, sx1, sy1);
  // Color colour = Color.Black;
  float dx = x1 - x0,
        dy = y1 - y0;
  float gradient = 1.0;
  if (dx != 0.0)
    gradient = dy / dx;
  int xpxl1, ypxl1;
  int xpxl2, ypxl2;
  // if (dy == 0) {


  //   if (floor(y0) != 640) {
  //     return;
  //   }
  //   printf("%.4f %.4f %.4f %.4f %d\n", x0, y0, x1, y1, switchXYaxis);
  // } else {
  //   return;
  // }

  float xend = round(x0);
  float yend = y0 + gradient * (xend - x0);
  float xgap = rfPart(x0 + 0.5);
  float intersectY = yend + gradient;
  xpxl1 = round(x0);
  ypxl1 = floor(yend);
  if (switchXYaxis) {
    set_sample_buf(ypxl1, xpxl1, convertColor(color, rfPart(yend) * xgap));
    set_sample_buf(ypxl1 + 1, xpxl1, convertColor(color, fPart(yend) * xgap));
  } else {
    set_sample_buf(xpxl1, ypxl1, convertColor(color, rfPart(yend) * xgap));
    set_sample_buf(xpxl1, ypxl1 + 1, convertColor(color, fPart(yend) * xgap));
  }

  xend = round(x1);
  yend = y1 + gradient * (xend - x1);
  xgap = fPart(x1 + 0.5);
  xpxl2 = xend;
  ypxl2 = floor(yend);
  if (switchXYaxis) {
    set_sample_buf(ypxl2, xpxl2, convertColor(color, rfPart(yend) * xgap));
    set_sample_buf(ypxl2+1, xpxl2,  convertColor(color, fPart(yend) * xgap));
  }
  else {
    set_sample_buf(xpxl2, ypxl2,  convertColor(color, rfPart(yend) * xgap));
    set_sample_buf(xpxl2, ypxl2+1, convertColor(color, fPart(yend) * xgap));
  }

  xpxl2 = round(x1);

  for (int x = xpxl1 + 1; x <= xpxl2 - 1; x++ )  {
    if (switchXYaxis) {
      set_sample_buf(floor(intersectY), x, convertColor(color, rfPart(intersectY)));
      set_sample_buf(floor(intersectY) + 1, x, convertColor(color, fPart(intersectY)));
    }
    else {
      set_sample_buf(x, floor(intersectY), convertColor(color, rfPart(intersectY)));
      set_sample_buf(x, floor(intersectY) + 1, convertColor(color, fPart(intersectY)));
    }
    intersectY += gradient;
  }
}

float cross(float x0, float y0, float x1, float y1, float x2, float y2) {
  return (x0 - x2) * (y1 - y2) - (y0 - y2) * (x1 - x2);
}

float delta = 1e-8;

int eps(float x) {
  if (x > delta) return 1;
  if (x < -delta) return -1;
  return 0;
}

bool inTriangle(float x, float y,
                float x0, float y0,
                float x1, float y1,
                float x2, float y2) {
  int d1 = eps(cross(x, y, x1, y1, x0, y0));
  int d2 = eps(cross(x, y, x2, y2, x1, y1));
  int d3 = eps(cross(x, y, x0, y0, x2, y2));
  return (((d1 >= 0) && (d2 >= 0) && (d3 >= 0)) ||
          ((d1 <= 0) && (d2 <= 0) && (d3 <= 0)));
}

float dist(float x0, float y0,
           float x1, float y1) {
  return sqrt((x0 - x1) * (x0 - x1) + (y0 - y1) * (y0 - y1));
}

void SoftwareRendererImp::rasterize_triangle( float x0, float y0,
                                              float x1, float y1,
                                              float x2, float y2,
                                              Color color ) {
  // Task 2:
  // Implement triangle rasterization

  x0 *= sample_rate;
  y0 *= sample_rate;
  x1 *= sample_rate;
  y1 *= sample_rate;
  x2 *= sample_rate;
  y2 *= sample_rate;


  float min_x = min(min(x0, x1), x2);
  float max_x = max(max(x0, x1), x2);
  float min_y = min(min(y0, y1), y2);
  float max_y = max(max(y0, y1), y2);

  for (int i = floor(min_x); i < round(max_x + 0.5); i++) {
    for (int j = floor(min_y) ; j < round(max_y + 0.5); j++) {
      if (inTriangle(i + 0.5, j + 0.5, x0, y0, x1, y1, x2, y2)) {
        set_sample_buf(i, j, color);
      }
    }
  }
}

void SoftwareRendererImp::rasterize_image( float x0, float y0,
                                           float x1, float y1,
                                           Texture& tex ) {
  // Task ?:
  // Implement image rasterization
  x0 *= sample_rate;
  y0 *= sample_rate;
  x1 *= sample_rate;
  y1 *= sample_rate;
  Color color;
  float xlen = x1 - x0;
  float ylen = y1 - y0;
  for (int x = floor(x0); x <= round(x1 + 0.5); x++) {
    for (int y = floor(y0); y <= round(y1 + 0.5); y++) {
      color = sampler->sample_nearest(tex, (x - x0) / xlen, (y - y0) / ylen);
      set_sample_buf(x, y, color);
    }
  }
  // printf("%.4f %.4f %.4f %.4f\n", x0, y0, x1, y1);

  // printf("%d %d\n", tex.width, tex.height);
}

// resolve samples to render target
void SoftwareRendererImp::resolve( void ) {
  clear_target();
  // Task 3:
  // Implement supersampling
  // You may also need to modify other functions marked with "Task 3";
  int sample_num = sample_rate * sample_rate;
  for (int sy = 0; sy < target_h; sy++) {
    for (int sx = 0; sx < target_w; sx++) {
      double sample_num = sample_rate * sample_rate;
      double a, rsum = 0, gsum = 0, bsum = 0;
      int render_target_start_point = 4 * (sx + sy * target_w);
      for (int y = 0; y < sample_rate; y++) {
        for (int x = 0; x < sample_rate; x++) {
          int buf_start_point = 4 * ((sy * sample_rate + y) * target_w * sample_rate + sx * sample_rate + x);
          // printf("%d %d %d %d %d %lu %d\n", target_w, sy, sx, y, x, super_sample_buffer.size(), buf_start_point);
          a = super_sample_buffer[buf_start_point + 3] * 1.0 / 255;
          rsum += super_sample_buffer[buf_start_point] * a;
          gsum += super_sample_buffer[buf_start_point + 1] * a;
          bsum += super_sample_buffer[buf_start_point + 2] * a;
        }
      }

      render_target[render_target_start_point] =  (uint8_t)(rsum / sample_num);
      render_target[render_target_start_point + 1] = (uint8_t)(gsum / sample_num);
      render_target[render_target_start_point + 2] = (uint8_t)(bsum / sample_num);
      render_target[render_target_start_point + 3] = 255;
      // (uint8_t)(render_target[render_target_start_point ] * a + (rsum / sample_num) * (1 - a));
      // render_target[render_target_start_point + 1] = (uint8_t)(render_target[render_target_start_point ] * a + (gsum / sample_num) * (1 - a));
      // render_target[render_target_start_point + 2] = (uint8_t)(render_target[render_target_start_point + 2] * a + (bsum / sample_num) * (1 - a));

    }
  }

  return;

}


} // namespace CMU462
