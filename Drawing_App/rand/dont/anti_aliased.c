#include <SDL2/SDL.h>
#include <stdint.h>
// #include <SDL2/SDL2_gfxPrimitives.h> // TODO:

int vlineRGBA(SDL_Renderer * renderer, Sint16 x, Sint16 y1, Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	int result = 0;
	result |= SDL_SetRenderDrawBlendMode(renderer, (a == 255) ? SDL_BLENDMODE_NONE : SDL_BLENDMODE_BLEND);
	result |= SDL_SetRenderDrawColor(renderer, r, g, b, a);
	result |= SDL_RenderDrawLine(renderer, x, y1, x, y2);
	return result;
}

int hlineRGBA(SDL_Renderer * renderer, Sint16 x1, Sint16 x2, Sint16 y, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	int result = 0;
	result |= SDL_SetRenderDrawBlendMode(renderer, (a == 255) ? SDL_BLENDMODE_NONE : SDL_BLENDMODE_BLEND);
	result |= SDL_SetRenderDrawColor(renderer, r, g, b, a);
	result |= SDL_RenderDrawLine(renderer, x1, y, x2, y);
	return result;
}

int pixelRGBA(SDL_Renderer * renderer, Sint16 x, Sint16 y, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	int result = 0;
	result |= SDL_SetRenderDrawBlendMode(renderer, (a == 255) ? SDL_BLENDMODE_NONE : SDL_BLENDMODE_BLEND);
	result |= SDL_SetRenderDrawColor(renderer, r, g, b, a);
	result |= SDL_RenderDrawPoint(renderer, x, y);
	return result;
}

int pixelRGBAWeight(SDL_Renderer * renderer, Sint16 x, Sint16 y, Uint8 r, Uint8 g, Uint8 b, Uint8 a, Uint32 weight)
{
	/*
	* Modify Alpha by weight
	*/
	Uint32 ax = a;
	ax = ((ax * weight) >> 8);
	if (ax > 255) {
		a = 255;
	} else {
		a = (Uint8)(ax & 0x000000ff);
	}

	return pixelRGBA(renderer, x, y, r, g, b, a);
}

int lineRGBA(SDL_Renderer * renderer, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	int result = 0;
	result |= SDL_SetRenderDrawBlendMode(renderer, (a == 255) ? SDL_BLENDMODE_NONE : SDL_BLENDMODE_BLEND);
	result |= SDL_SetRenderDrawColor(renderer, r, g, b, a);
	result |= SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
	return result;
}

void _aalineRGBA(SDL_Renderer * renderer, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a, int draw_endpoint)
{
        #define AAlevels 256
        #define AAbits 8

	Sint32 xx0, yy0, xx1, yy1;
	Uint32 intshift, erracc, erradj;
	Uint32 erracctmp, wgt;
	int dx, dy, tmp, xdir, y0p1, x0pxdir;

	/*
	* Keep on working with 32bit numbers
	*/
	xx0 = x1;
	yy0 = y1;
	xx1 = x2;
	yy1 = y2;

	/*
	* Reorder points to make dy positive
	*/
	if (yy0 > yy1) {
		tmp = yy0;
		yy0 = yy1;
		yy1 = tmp;
		tmp = xx0;
		xx0 = xx1;
		xx1 = tmp;
	}

	/*
	* Calculate distance
	*/
	dx = xx1 - xx0;
	dy = yy1 - yy0;

	/*
	* Adjust for negative dx and set xdir
	*/
	if (dx >= 0) {
		xdir = 1;
	} else {
		xdir = -1;
		dx = (-dx);
	}

	/*
	* Check for special cases
	*/
	if (dx == 0) {
		/*
		* Vertical line
		*/
		if (draw_endpoint)
		{
        		vlineRGBA(renderer, x1, y1, y2, r, g, b, a);
			return;
		} else {
			if (dy > 0) {
        			vlineRGBA(renderer, x1, yy0, yy0+dy, r, g, b, a);
				return;
			} else {
        			pixelRGBA(renderer, x1, y1, r, g, b, a);
				return;
			}
		}
	} else if (dy == 0) {
		/*
		* Horizontal line
		*/
		if (draw_endpoint)
		{
		        hlineRGBA(renderer, x1, x2, y1, r, g, b, a);
			return;
		} else {
			if (dx > 0) {
        			hlineRGBA(renderer, xx0, xx0+dx, y1, r, g, b, a);
				return;
			} else {
        			pixelRGBA(renderer, x1, y1, r, g, b, a);
				return;
			}
		}
	} else if ((dx == dy) && (draw_endpoint)) {
		/*
		* Diagonal line (with endpoint)
		*/
		lineRGBA(renderer, x1, y1, x2, y2,  r, g, b, a);
		return;
	}


	/*
	* Line is not horizontal, vertical or diagonal (with endpoint)
	*/

	/*
	* Zero accumulator
	*/
	erracc = 0;

	/*
	* # of bits by which to shift erracc to get intensity level
	*/
	intshift = 32 - AAbits;

	/*
	* Draw the initial pixel in the foreground color
	*/
	pixelRGBA(renderer, x1, y1, r, g, b, a);

	/*
	* x-major or y-major?
	*/
	if (dy > dx) {

		/*
		* y-major.  Calculate 16-bit fixed point fractional part of a pixel that
		* X advances every time Y advances 1 pixel, truncating the result so that
		* we won't overrun the endpoint along the X axis
		*/
		erradj = ((dx << 16) / dy) << 16;

		/*
		* draw all pixels other than the first and last
		*/
		x0pxdir = xx0 + xdir;
		while (--dy) {
			erracctmp = erracc;
			erracc += erradj;
			if (erracc <= erracctmp) {
				/*
				* rollover in error accumulator, x coord advances
				*/
				xx0 = x0pxdir;
				x0pxdir += xdir;
			}
			yy0++;		/* y-major so always advance Y */

			/*
			* the AAbits most significant bits of erracc give us the intensity
			* weighting for this pixel, and the complement of the weighting for
			* the paired pixel.
			*/
			wgt = (erracc >> intshift) & 255;
			pixelRGBAWeight (renderer, xx0, yy0, r, g, b, a, 255 - wgt);
			pixelRGBAWeight (renderer, x0pxdir, yy0, r, g, b, a, wgt);
		}

	} else {

		/*
		* x-major line.  Calculate 16-bit fixed-point fractional part of a pixel
		* that Y advances each time X advances 1 pixel, truncating the result so
		* that we won't overrun the endpoint along the X axis.
		*/
		erradj = ((dy << 16) / dx) << 16;

		/*
		* draw all pixels other than the first and last
		*/
		y0p1 = yy0 + 1;
		while (--dx) {

			erracctmp = erracc;
			erracc += erradj;
			if (erracc <= erracctmp) {
				/*
				* Accumulator turned over, advance y
				*/
				yy0 = y0p1;
				y0p1++;
			}
			xx0 += xdir;	/* x-major so always advance X */
			/*
			* the AAbits most significant bits of erracc give us the intensity
			* weighting for this pixel, and the complement of the weighting for
			* the paired pixel.
			*/
			wgt = (erracc >> intshift) & 255;
			pixelRGBAWeight (renderer, xx0, yy0, r, g, b, a, 255 - wgt);
			pixelRGBAWeight (renderer, xx0, y0p1, r, g, b, a, wgt);
		}
	}

	/*
	* Do we have to draw the endpoint
	*/
	if (draw_endpoint) {
		pixelRGBA (renderer, x2, y2, r, g, b, a);
	}

	return;
}
