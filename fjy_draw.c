#include "../common/common.h"
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <string.h>

static int LCD_FB_FD;
static int* LCD_FB_BUF = NULL;
static int DRAW_BUF[SCREEN_WIDTH * SCREEN_HEIGHT];

static struct area {
	int x1, x2, y1, y2;
} update_area = { 0,0,0,0 };

#define AREA_SET_EMPTY(pa) do {\
	(pa)->x1 = SCREEN_WIDTH;\
	(pa)->x2 = 0;\
	(pa)->y1 = SCREEN_HEIGHT;\
	(pa)->y2 = 0;\
} while(0)

void fb_init(char* dev)
{
	int fd;
	struct fb_fix_screeninfo fb_fix;
	struct fb_var_screeninfo fb_var;

	if (LCD_FB_BUF != NULL) return; /*already done*/

	//进入终端图形模式
	fd = open("/dev/tty0", O_RDWR, 0);
	ioctl(fd, KDSETMODE, KD_GRAPHICS);
	close(fd);

	//First: Open the device
	if ((fd = open(dev, O_RDWR)) < 0) {
		printf("Unable to open framebuffer %s, errno = %d\n", dev, errno);
		return;
	}
	if (ioctl(fd, FBIOGET_FSCREENINFO, &fb_fix) < 0) {
		printf("Unable to FBIOGET_FSCREENINFO %s\n", dev);
		return;
	}
	if (ioctl(fd, FBIOGET_VSCREENINFO, &fb_var) < 0) {
		printf("Unable to FBIOGET_VSCREENINFO %s\n", dev);
		return;
	}

	printf("framebuffer info: bits_per_pixel=%u,size=(%d,%d),virtual_pos_size=(%d,%d)(%d,%d),line_length=%u,smem_len=%u\n",
		fb_var.bits_per_pixel, fb_var.xres, fb_var.yres, fb_var.xoffset, fb_var.yoffset,
		fb_var.xres_virtual, fb_var.yres_virtual, fb_fix.line_length, fb_fix.smem_len);

	//Second: mmap
	void* addr = mmap(NULL, fb_fix.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (addr == (void*)-1) {
		printf("failed to mmap memory for framebuffer.\n");
		return;
	}

	if ((fb_var.xoffset != 0) || (fb_var.yoffset != 0))
	{
		fb_var.xoffset = 0;
		fb_var.yoffset = 0;
		if (ioctl(fd, FBIOPAN_DISPLAY, &fb_var) < 0) {
			printf("FBIOPAN_DISPLAY framebuffer failed\n");
		}
	}

	LCD_FB_FD = fd;
	LCD_FB_BUF = addr;

	//set empty
	AREA_SET_EMPTY(&update_area);
	return;
}

static void _copy_area(int* dst, int* src, struct area* pa)
{
	int x, y, w, h;
	x = pa->x1; w = pa->x2 - x;
	y = pa->y1; h = pa->y2 - y;
	src += y * SCREEN_WIDTH + x;
	dst += y * SCREEN_WIDTH + x;
	while (h-- > 0) {
		memcpy(dst, src, w * 4);
		src += SCREEN_WIDTH;
		dst += SCREEN_WIDTH;
	}
}

static int _check_area(struct area* pa)
{
	if (pa->x2 == 0) return 0; //is empty

	if (pa->x1 < 0) pa->x1 = 0;
	if (pa->x2 > SCREEN_WIDTH) pa->x2 = SCREEN_WIDTH;
	if (pa->y1 < 0) pa->y1 = 0;
	if (pa->y2 > SCREEN_HEIGHT) pa->y2 = SCREEN_HEIGHT;

	if ((pa->x2 > pa->x1) && (pa->y2 > pa->y1))
		return 1; //no empty

	//set empty
	AREA_SET_EMPTY(pa);
	return 0;
}

void fb_update(void)
{
	if (_check_area(&update_area) == 0) return; //is empty
	_copy_area(LCD_FB_BUF, DRAW_BUF, &update_area);
	AREA_SET_EMPTY(&update_area); //set empty
	return;
}

/*======================================================================*/

static void* _begin_draw(int x, int y, int w, int h)
{
	int x2 = x + w;
	int y2 = y + h;
	if (update_area.x1 > x) update_area.x1 = x;
	if (update_area.y1 > y) update_area.y1 = y;
	if (update_area.x2 < x2) update_area.x2 = x2;
	if (update_area.y2 < y2) update_area.y2 = y2;
	return DRAW_BUF;
}

void fb_draw_pixel(int x, int y, int color)
{
	if (x < 0 || y < 0 || x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT) return;
	int* buf = _begin_draw(x, y, 1, 1);
	/*---------------------------------------------------*/
	*(buf + y * SCREEN_WIDTH + x) = color;
	/*---------------------------------------------------*/
	return;
}

void fb_draw_rect(int x, int y, int w, int h, int color)
{
	if (x < 0) { w += x; x = 0; }
	if (x + w > SCREEN_WIDTH) { w = SCREEN_WIDTH - x; }
	if (y < 0) { h += y; y = 0; }
	if (y + h > SCREEN_HEIGHT) { h = SCREEN_HEIGHT - y; }
	if (w <= 0 || h <= 0) return;
	int* buf = _begin_draw(x, y, w, h);
	/*---------------------------------------------------*/
		//printf("you need implement fb_draw_rect()\n"); exit(0);
	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			*(buf + (y + i) * SCREEN_WIDTH + (x + j)) = color;
		}
	}
	/*---------------------------------------------------*/
	return;
}

void fb_draw_line(int x1, int y1, int x2, int y2, int color)
{
	/*---------------------------------------------------*/
		//printf("you need implement fb_draw_line()\n"); exit(0);
	int dx;
	if (x2 > x1) dx = x2 - x1;
	else dx = x1 - x2;
	int dy;
	if (y2 > y1) dy = y2 - y1;
	else dy = y1 - y2;
	int sx = (x1 < x2) ? 1 : -1;
	int sy = (y1 < y2) ? 1 : -1;
	int err = dx - dy;

	int* buf = _begin_draw(x1, y1, 1, 1);
	while (1) {
		// 绘制像素点
		if (x1 < 0 || y1 < 0 || x1 >= SCREEN_WIDTH || y1 >= SCREEN_HEIGHT) return;
		*(buf + y1 * SCREEN_WIDTH + x1) = color;


		if (x1 == x2 && y1 == y2) break;
		int e2 = err * 2;
		if (e2 > -dy) { err -= dy; x1 += sx; }
		if (e2 < dx) { err += dx; y1 += sy; }
	}
	/*---------------------------------------------------*/
	return;
}

void fb_draw_circle(int x, int y, int r, int color)
{
	if (r <= 0) return;

	// 计算圆的边界框
	int x1 = x - r;
	int x2 = x + r;
	int y1 = y - r;
	int y2 = y + r;

	// 裁剪到屏幕范围
	if (x1 < 0) x1 = 0;
	if (x2 > SCREEN_WIDTH) x2 = SCREEN_WIDTH;
	if (y1 < 0) y1 = 0;
	if (y2 > SCREEN_HEIGHT) y2 = SCREEN_HEIGHT;

	// 更新绘制区域
	int* buf = _begin_draw(x1, y1, x2 - x1, y2 - y1);

	// 预先计算 r² 的值
	int r_squared = r * r;

	// 填充实心圆
	for (int j = y1; j < y2; j++) {
		// 计算当前行的偏移量
		int* row_ptr = buf + j * SCREEN_WIDTH;

		for (int i = x1; i < x2; i++) {
			// 计算点到圆心的距离平方
			int dx = i - x;
			int dy = j - y;
			int dist_squared = dx * dx + dy * dy;

			// 如果点在圆内（包括边界），则绘制
			if (dist_squared <= r_squared) {
				row_ptr[i] = color;
			}
		}
	}
}


/**
 * 绘制半圆
 * @param direction 方向：0-上，1-下，2-左，3-右
 */
void fb_draw_half_circle(int x, int y, int r, int color, int direction)
{
	if (r <= 0) return;

	// 根据方向计算实际的边界框，减少不必要的循环
	int x1, x2, y1, y2;

	switch (direction) {
	case 0: // 上
		x1 = x - r;
		x2 = x + r;
		y1 = y - r;
		y2 = y;  // 只到圆心高度
		break;
	case 1: // 下
		x1 = x - r;
		x2 = x + r;
		y1 = y;
		y2 = y + r;  // 从圆心开始
		break;
	case 2: // 左
		x1 = x - r;
		x2 = x;  // 只到圆心宽度
		y1 = y - r;
		y2 = y + r;
		break;
	case 3: // 右
		x1 = x;
		x2 = x + r;  // 从圆心开始
		y1 = y - r;
		y2 = y + r;
		break;
	default:
		x1 = x - r;
		x2 = x + r;
		y1 = y - r;
		y2 = y;
		break;
	}

	// 裁剪到屏幕范围
	if (x1 < 0) x1 = 0;
	if (x2 > SCREEN_WIDTH) x2 = SCREEN_WIDTH;
	if (y1 < 0) y1 = 0;
	if (y2 > SCREEN_HEIGHT) y2 = SCREEN_HEIGHT;

	// 如果裁剪后区域无效，直接返回
	if (x1 >= x2 || y1 >= y2) return;

	// 更新绘制区域
	int* buf = _begin_draw(x1, y1, x2 - x1, y2 - y1);

	// 预先计算 r² 的值
	int r_squared = r * r;

	// 根据方向绘制半圆
	for (int j = y1; j < y2; j++) {
		int* row_ptr = buf + j * SCREEN_WIDTH;

		for (int i = x1; i < x2; i++) {
			int dx = i - x;
			int dy = j - y;
			int dist_squared = dx * dx + dy * dy;

			if (dist_squared <= r_squared) {
				row_ptr[i] = color;
			}
		}
	}
}




void fb_draw_image(int x, int y, fb_image* image, int color)
{
	if (image == NULL) return;

	int ix = 0; //image x
	int iy = 0; //image y
	int w = image->pixel_w; //draw width
	int h = image->pixel_h; //draw height

	if (x < 0) { w += x; ix -= x; x = 0; }
	if (y < 0) { h += y; iy -= y; y = 0; }

	if (x + w > SCREEN_WIDTH) {
		w = SCREEN_WIDTH - x;
	}
	if (y + h > SCREEN_HEIGHT) {
		h = SCREEN_HEIGHT - y;
	}
	if ((w <= 0) || (h <= 0)) return;

	int* buf = _begin_draw(x, y, w, h);
	/*---------------------------------------------------------------*/
	char* dst = (char*)(buf + y * SCREEN_WIDTH + x);
	char* src; //不同的图像颜色格式定位不同
	/*---------------------------------------------------------------*/

	int alpha;
	int ww;

	if (image->color_type == FB_COLOR_RGB_8880) /*lab3: jpg*/
	{
		//printf("you need implement fb_draw_image() FB_COLOR_RGB_8880\n"); exit(0);

		// 计算源图像数据的起始位置（考虑裁剪）
		src = image->content + (iy * image->pixel_w + ix) * 4;

		for (int i = 0; i < h; i++) {
			for (int j = 0; j < w; j++) {
				char* src_pixel = src + (i * image->pixel_w + j) * 4;

				// 获取RGB分量
				unsigned char b = *src_pixel;
				unsigned char g = *(++src_pixel);
				unsigned char r = *(++src_pixel);
				unsigned int argb = 0xFF000000 | (r << 16) | (g << 8) | b;

				int* dst_pixel = (int*)(dst + (i * SCREEN_WIDTH + j) * 4);

				*dst_pixel = argb;
			}
		}


		return;
	}
	else if (image->color_type == FB_COLOR_RGBA_8888) /*lab3: png*/
	{
		//printf("you need implement fb_draw_image() FB_COLOR_RGBA_8888\n"); exit(0);

		// 计算源图像数据的起始位置（考虑裁剪）
		src = image->content + (iy * image->pixel_w + ix) * 4;

		for (int i = 0; i < h; i++) {
			for (int j = 0; j < w; j++) {
				// 获取png图ARGB分量
				char* src_pixel = src + (i * image->pixel_w + j) * 4;
				unsigned char B1 = *src_pixel;
				unsigned char G1 = *(++src_pixel);
				unsigned char R1 = *(++src_pixel);
				unsigned char alpha = *(++src_pixel);
				//获取原图层RGB分量
				int* dst_pixel = (int*)(dst + (i * SCREEN_WIDTH + j) * 4);
				unsigned char B2 = (*dst_pixel & 0x000000FF);
				unsigned char G2 = (*dst_pixel & 0x0000FF00) >> 8;
				unsigned char R2 = (*dst_pixel & 0x00FF0000) >> 16;
				// 和上一个图层混合
				// 不能先算alpha / 255，因为一定等于0，为了性能又不好使用float
				unsigned char R = R1 * alpha / 255 + R2 * (255 - alpha) / 255;
				unsigned char G = G1 * alpha / 255 + G2 * (255 - alpha) / 255;
				unsigned char B = B1 * alpha / 255 + B2 * (255 - alpha) / 255;
				unsigned int argb = 0xFF000000 | (R << 16) | (G << 8) | B;

				*dst_pixel = argb;
			}
		}

		return;
	}
	else if (image->color_type == FB_COLOR_ALPHA_8) /*lab3: font*/
	{
		//printf("you need implement fb_draw_image() FB_COLOR_ALPHA_8\n"); exit(0);

		// 计算源图像数据的起始位置（考虑裁剪）
		src = image->content + (iy * image->pixel_w + ix) * 1;

		for (int i = 0; i < h; i++) {
			for (int j = 0; j < w; j++) {
				// 获取字体图的rgb和透明度
				char* src_pixel = src + (i * image->pixel_w + j) * 1;
				unsigned char B1 = (color & 0x000000FF);
				unsigned char G1 = (color & 0x0000FF00) >> 8;
				unsigned char R1 = (color & 0x00FF0000) >> 16;
				unsigned char alpha = *src_pixel;
				//获取原图层RGB分量
				int* dst_pixel = (int*)(dst + (i * SCREEN_WIDTH + j) * 4);
				unsigned char B2 = (*dst_pixel & 0x000000FF);
				unsigned char G2 = (*dst_pixel & 0x0000FF00) >> 8;
				unsigned char R2 = (*dst_pixel & 0x00FF0000) >> 16;
				// 和上一个图层混合
				// 不能先算alpha / 255，因为一定等于0，为了性能又不好使用float
				unsigned char R = R1 * alpha / 255 + R2 * (255 - alpha) / 255;
				unsigned char G = G1 * alpha / 255 + G2 * (255 - alpha) / 255;
				unsigned char B = B1 * alpha / 255 + B2 * (255 - alpha) / 255;
				unsigned int argb = 0xFF000000 | (R << 16) | (G << 8) | B;

				*dst_pixel = argb;
			}
		}

		return;
	}
	/*---------------------------------------------------------------*/
	return;
}

void fb_draw_border(int x, int y, int w, int h, int color)
{
	if (w <= 0 || h <= 0) return;
	fb_draw_rect(x, y, w, 1, color);
	if (h > 1) {
		fb_draw_rect(x, y + h - 1, w, 1, color);
		fb_draw_rect(x, y + 1, 1, h - 2, color);
		if (w > 1) fb_draw_rect(x + w - 1, y + 1, 1, h - 2, color);
	}
}

/** draw a text string **/
void fb_draw_text(int x, int y, char* text, int font_size, int color)
{
	fb_image* img;
	fb_font_info info;
	int i = 0;
	int len = strlen(text);
	while (i < len)
	{
		img = fb_read_font_image(text + i, font_size, &info);
		if (img == NULL) {
			printf("font not found");
			break;
		}
		fb_draw_image(x + info.left, y - info.top, img, color);
		fb_free_image(img);

		x += info.advance_x;
		i += info.bytes;
	}
	return;
}

