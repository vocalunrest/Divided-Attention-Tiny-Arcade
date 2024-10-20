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

void drawTriangle(int x0, int y0, int height, int color, bool fill) {
    // Calculate the side length using integer approximation
    // s = (2 * h) / sqrt(3)
    // Approximate sqrt(3) as 1732/1000
    int size = (2 * height * 1000) / 1732;

    // Calculate the coordinates of the vertices
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
