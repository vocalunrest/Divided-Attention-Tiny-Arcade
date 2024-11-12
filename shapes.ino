void drawCircle(int x0, int y0, int radius, int color, bool fill) {
    int x = 0;
    int y = radius;
    int d = 1 - radius;

    while (x <= y) {
        if (fill) {
            // Draw horizontal lines to fill the circle
            display.drawLine(x0 - x, y0 + y, x0 + x, y0 + y, color);
            display.drawLine(x0 - y, y0 + x, x0 + y, y0 + x, color);
            display.drawLine(x0 - x, y0 - y, x0 + x, y0 - y, color);
            display.drawLine(x0 - y, y0 - x, x0 + y, y0 - x, color);
        } else {
            // Draw the circle's perimeter
            display.drawPixel(x0 + x, y0 + y, color);
            display.drawPixel(x0 + y, y0 + x, color);
            display.drawPixel(x0 - x, y0 + y, color);
            display.drawPixel(x0 - y, y0 + x, color);
            display.drawPixel(x0 + x, y0 - y, color);
            display.drawPixel(x0 + y, y0 - x, color);
            display.drawPixel(x0 - x, y0 - y, color);
            display.drawPixel(x0 - y, y0 - x, color);
        }

        if (d < 0) {
            d += 2 * x + 3;
        } else {
            d += 2 * (x - y) + 5;
            y--;
        }
        x++;
    }
}

void drawRoundedRect(int x, int y, int width, int height, int radius, int color, bool fill) {
    if (fill) {
        // Fill the central rectangle
        fillRect(x + radius, y, width - 2 * radius, height, color);

        // Fill the side rectangles
        fillRect(x, y + radius, radius, height - 2 * radius, color);
        fillRect(x + width - radius, y + radius, radius, height - 2 * radius, color);

        // Fill the four rounded corners
        fillCircle(x + radius, y + radius, radius, color);
        fillCircle(x + width - radius - 1, y + radius, radius, color);
        fillCircle(x + radius, y + height - radius - 1, radius, color);
        fillCircle(x + width - radius - 1, y + height - radius - 1, radius, color);
    } else {
        // Outline the rectangle if not filling
        display.drawLine(x + radius, y, x + width - radius - 1, y, color);
        display.drawLine(x + radius, y + height - 1, x + width - radius - 1, y + height - 1, color);
        display.drawLine(x, y + radius, x, y + height - radius - 1, color);
        display.drawLine(x + width - 1, y + radius, x + width - 1, y + height - radius - 1, color);

        // Draw the four rounded corners
        drawCircleHelper(x + radius, y + radius, radius, 1, color); // Top-left corner
        drawCircleHelper(x + width - radius - 1, y + radius, radius, 2, color); // Top-right corner
        drawCircleHelper(x + width - radius - 1, y + height - radius - 1, radius, 4, color); // Bottom-right corner
        drawCircleHelper(x + radius, y + height - radius - 1, radius, 8, color); // Bottom-left corner
    }
}

void drawCircleHelper(int x0, int y0, int r, uint8_t cornername, int color) {
    int f = 1 - r;
    int ddF_x = 1;
    int ddF_y = -2 * r;
    int x = 0;
    int y = r;

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        if (cornername & 0x1) {
            display.drawPixel(x0 - y, y0 - x, color);
            display.drawPixel(x0 - x, y0 - y, color);
        }
        if (cornername & 0x2) {
            display.drawPixel(x0 + x, y0 - y, color);
            display.drawPixel(x0 + y, y0 - x, color);
        }
        if (cornername & 0x4) {
            display.drawPixel(x0 + x, y0 + y, color);
            display.drawPixel(x0 + y, y0 + x, color);
        }
        if (cornername & 0x8) {
            display.drawPixel(x0 - y, y0 + x, color);
            display.drawPixel(x0 - x, y0 + y, color);
        }
    }
}


void fillRect(int x, int y, int width, int height, int color) {
    for (int i = x; i < x + width; i++) {
        for (int j = y; j < y + height; j++) {
            display.drawPixel(i, j, color);
        }
    }
}

void fillCircle(int x0, int y0, int radius, int color) {
    int x = radius;
    int y = 0;
    int err = 0;

    while (x >= y) {
        for (int i = x0 - x; i <= x0 + x; i++) {
            display.drawPixel(i, y0 + y, color);
            display.drawPixel(i, y0 - y, color);
        }
        for (int i = x0 - y; i <= x0 + y; i++) {
            display.drawPixel(i, y0 + x, color);
            display.drawPixel(i, y0 - x, color);
        }

        y += 1;
        if (err <= 0) {
            err += 2 * y + 1;
        } else {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

// void drawRoundedTriangle(int x0, int y0, int height, int radius, int color, bool fill) {
//     // Calculate the side length using integer approximation
//     // s = (2 * h) / sqrt(3)
//     // Approximate sqrt(3) as 1732/1000
//     int size = (2 * height * 1000) / 1732;

//     // Calculate the coordinates of the vertices
//     int x1 = x0;
//     int y1 = y0 - (2 * height) / 3; // Top vertex

//     int x2 = x0 - size / 2;
//     int y2 = y0 + height / 3; // Bottom left vertex

//     int x3 = x0 + size / 2;
//     int y3 = y0 + height / 3; // Bottom right vertex

//     if (fill) {
//         // Find the bounding rectangle of the triangle
//         int minY = min(y1, min(y2, y3));
//         int maxY = max(y1, max(y2, y3));

//         // Loop over each scanline within the triangle's bounding box
//         for (int y = minY; y <= maxY; y++) {
//             // Initialize intersection points
//             int xStart = x0;
//             int xEnd = x0;

//             // Edge from Vertex 1 to Vertex 2
//             if ((y - y1) * (y - y2) <= 0 && y1 != y2) {
//                 int x = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
//                 xStart = min(xStart, x);
//                 xEnd = max(xEnd, x);
//             }

//             // Edge from Vertex 2 to Vertex 3
//             if ((y - y2) * (y - y3) <= 0 && y2 != y3) {
//                 int x = x2 + (x3 - x2) * (y - y2) / (y3 - y2);
//                 xStart = min(xStart, x);
//                 xEnd = max(xEnd, x);
//             }

//             // Edge from Vertex 3 to Vertex 1
//             if ((y - y3) * (y - y1) <= 0 && y3 != y1) {
//                 int x = x3 + (x1 - x3) * (y - y3) / (y1 - y3);
//                 xStart = min(xStart, x);
//                 xEnd = max(xEnd, x);
//             }

//             // Draw a horizontal line between xStart and xEnd
//             display.drawLine(xStart, y, xEnd, y, color);
//         }

//         // Fill the rounded corners
//         fillCircle(x1, y1, radius, color);
//         fillCircle(x2, y2, radius, color);
//         fillCircle(x3, y3, radius, color);
//     } else {
//         // Draw the outline of the triangle
//         display.drawLine(x1 + radius, y1, x2 - radius, y2, color); // Edge from Vertex 1 to Vertex 2
//         display.drawLine(x2 + radius, y2, x3 - radius, y3, color); // Edge from Vertex 2 to Vertex 3
//         display.drawLine(x3 + radius, y3, x1 - radius, y1, color); // Edge from Vertex 3 to Vertex 1

//         // Draw the rounded corners
//         drawTriangleArc(x1, y1, radius, 1, color); // Top vertex
//         drawTriangleArc(x2, y2, radius, 2, color); // Bottom left vertex
//         drawTriangleArc(x3, y3, radius, 4, color); // Bottom right vertex
//     }
// }

// void drawTriangleArc(int x0, int y0, int r, uint8_t cornername, int color) {
//     int color = TS_8b_Green;
//     int f = 1 - r;
//     int ddF_x = 1;
//     int ddF_y = -2 * r;
//     int x = 0;
//     int y = r;

//     while (x < y) {
//         if (f >= 0) {
//             y--;
//             ddF_y += 2;
//             f += ddF_y;
//         }
//         x++;
//         ddF_x += 2;
//         f += ddF_x;

//         if (cornername & 0x1) {
//             display.drawPixel(x0 - y, y0 - x, color);
//             display.drawPixel(x0 - x, y0 - y, color);
//         }
//         if (cornername & 0x2) {
//             display.drawPixel(x0 + x, y0 - y, color);
//             display.drawPixel(x0 + y, y0 - x, color);
//         }
//         if (cornername & 0x4) {
//             display.drawPixel(x0 + x, y0 + y, color);
//             display.drawPixel(x0 + y, y0 + x, color);
//         }
//         if (cornername & 0x8) {
//             display.drawPixel(x0 - y, y0 + x, color);
//             display.drawPixel(x0 - x, y0 + y, color);
//         }
//     }
// }


void drawTriangle(int x0, int y0, int height, int color, bool fill) {
    // Calculate the side length using integer approximation
    // s = (2 * h) / sqrt(3)
    // Approximate sqrt(3) as 1732/1000
    int size = (2 * height * 1000) / 1732;

    // the coordinates of the vertices
    int x1 = x0;
    int y1 = y0 - (2 * height) / 3; // Top vertex

    int x2 = x0 - size / 2;
    int y2 = y0 + height / 3; // Bottom left vertex

    int x3 = x0 + size / 2;
    int y3 = y0 + height / 3; // Bottom right vertex

    if (fill) {
        // Find the bounding rectangle of the triangle
        int minY = min(y1, min(y2, y3));
        int maxY = max(y1, max(y2, y3));

        // Loop over each scanline within the triangle's bounding box
        for (int y = minY; y <= maxY; y++) {
            // Initialize intersection points
            int xStart = x0;
            int xEnd = x0;

            // Edge from Vertex 1 to Vertex 2
            if ((y - y1) * (y - y2) <= 0 && y1 != y2) {
                int x = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
                xStart = min(xStart, x);
                xEnd = max(xEnd, x);
            }

            // Edge from Vertex 2 to Vertex 3
            if ((y - y2) * (y - y3) <= 0 && y2 != y3) {
                int x = x2 + (x3 - x2) * (y - y2) / (y3 - y2);
                xStart = min(xStart, x);
                xEnd = max(xEnd, x);
            }

            // Edge from Vertex 3 to Vertex 1
            if ((y - y3) * (y - y1) <= 0 && y3 != y1) {
                int x = x3 + (x1 - x3) * (y - y3) / (y1 - y3);
                xStart = min(xStart, x);
                xEnd = max(xEnd, x);
            }

            // Draw a horizontal line between xStart and xEnd
            display.drawLine(xStart, y, xEnd, y, color);
        }
    } else {
        // Draw the outline of the triangle
        display.drawLine(x1, y1, x2, y2, color); // Edge from Vertex 1 to Vertex 2
        display.drawLine(x2, y2, x3, y3, color); // Edge from Vertex 2 to Vertex 3
        display.drawLine(x3, y3, x1, y1, color); // Edge from Vertex 3 to Vertex 1
    }
}

void drawPlus(int x, int y, int size, int thickness, int color, bool fill) {
    int halfSize = size / 2;
    int halfThickness = thickness / 2;

    if (fill) {
        // Fill the vertical rectangle
        fillRect(x - halfThickness, y - halfSize, thickness, size, color);

        // Fill the horizontal rectangle
        fillRect(x - halfSize, y - halfThickness, size, thickness, color);
    } else {
        // Outline the vertical rectangle
        drawRect(x - halfThickness, y - halfSize, thickness, size, color);

        // Outline the horizontal rectangle
        drawRect(x - halfSize, y - halfThickness, size, thickness, color);
    }
}

void drawRect(int x, int y, int width, int height, int color) {
    display.drawLine(x, y, x + width - 1, y, color);
    display.drawLine(x, y + height - 1, x + width - 1, y + height - 1, color);
    display.drawLine(x, y, x, y + height - 1, color);
    display.drawLine(x + width - 1, y, x + width - 1, y + height - 1, color);
}

void drawStar(int x, int y, int size, int color, bool fill) {
    // Calculate the coordinates of the vertices
    float angle = PI / 5; // 36 degrees
    int outerRadius = size / 2;
    int innerRadius = outerRadius * 0.5; 
    int vertices[10][2];
    for (int i = 0; i < 10; i++) {
        float r = (i % 2 == 0) ? outerRadius : innerRadius;
        vertices[i][0] = x + r * cos(i * angle - PI / 2);
        vertices[i][1] = y + r * sin(i * angle - PI / 2);
    }

    if (fill) {
        // Fill the star by drawing triangles between the center and the vertices
        for (int i = 0; i < 10; i++) {
            int next = (i + 1) % 10;
            fillTriangle(x, y, vertices[i][0], vertices[i][1], vertices[next][0], vertices[next][1], color);
        }
    } else {
        // Outline the star by drawing lines between the vertices
        for (int i = 0; i < 10; i++) {
            int next = (i + 1) % 10;
            display.drawLine(vertices[i][0], vertices[i][1], vertices[next][0], vertices[next][1], color);
        }
    }
}

void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, int color) {
    // Find the bounding box of the triangle
    int minX = min(x0, min(x1, x2));
    int maxX = max(x0, max(x1, x2));
    int minY = min(y0, min(y1, y2));
    int maxY = max(y0, max(y1, y2));

    // Loop over each scanline within the triangle's bounding box
    for (int y = minY; y <= maxY; y++) {
        // Initialize intersection points
        int xStart = maxX;
        int xEnd = minX;

        // Edge from Vertex 0 to Vertex 1
        if ((y - y0) * (y - y1) <= 0 && y0 != y1) {
            int x = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
            xStart = min(xStart, x);
            xEnd = max(xEnd, x);
        }

        // Edge from Vertex 1 to Vertex 2
        if ((y - y1) * (y - y2) <= 0 && y1 != y2) {
            int x = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
            xStart = min(xStart, x);
            xEnd = max(xEnd, x);
        }

        // Edge from Vertex 2 to Vertex 0
        if ((y - y2) * (y - y0) <= 0 && y2 != y0) {
            int x = x2 + (x0 - x2) * (y - y2) / (y0 - y2);
            xStart = min(xStart, x);
            xEnd = max(xEnd, x);
        }

        // Draw a horizontal line between xStart and xEnd
        display.drawLine(xStart, y, xEnd, y, color);
    }
}