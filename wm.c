#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MAX_WINDOWS 100
static const int padding = 10;

void adopt_current(Display *dpy, Window managed_windows[], int *window_count){
	Window root = DefaultRootWindow(dpy);
	Window parent_return, *children_return;
	unsigned int num_children;

	if (XQueryTree(dpy, root, &root, &parent_return, &children_return, &num_children)) {
		for (unsigned int i = 0; i < num_children; i++) {
			XWindowAttributes attrs;
			XGetWindowAttributes(dpy, children_return[i], &attrs);

			if (!attrs.override_redirect && attrs.map_state == IsViewable) {
				if (*window_count < MAX_WINDOWS) {
					managed_windows[*window_count] = children_return[i];
					(*window_count)++;
				}
			}
		}
		if (children_return != NULL) {
			XFree(children_return);
		}
	}
}

void filter_windows(Window old_windows[], int old_count, Window filtered_windows[], int *new_count, Window dead_win){
	int new_index = 0;
	for(int i=0; i<old_count; i++){
		if(old_windows[i] == dead_win) continue;
		filtered_windows[new_index] = old_windows[i];
		new_index++;
	}
	*new_count = new_index;
}

void draw_windows(Display *dpy, int screen_w, int screen_h, Window managed_windows[], int window_count, int padding){
	if(window_count==1){
		XMoveResizeWindow(dpy, managed_windows[0], padding, padding, screen_w-2*padding, screen_h-2*padding);
	}else{
		Window root_win = managed_windows[0];
		XMoveResizeWindow(dpy, root_win, padding, padding, screen_w/2-padding, screen_h-2*padding);

		int denominator = (window_count-1==0) ? 1 : window_count-1;
		int usable_h = screen_h-padding*window_count;
		int base_h = usable_h/denominator;
		int current_y = padding;
		int remainder = usable_h % denominator;
		for(int i=1; i<window_count; i++){
			Window win = managed_windows[i];
			int win_h;
			if(remainder>0){
				win_h = base_h + 1;
				remainder--;
			}else{
				win_h = base_h;
			}
				
			XMoveResizeWindow(dpy, win, screen_w/2+padding, current_y, screen_w/2-2*padding, win_h);
			current_y+=win_h + padding;
		}
	}
}

int main(void)
{
    Display * dpy;
    XWindowAttributes attrs;
    XButtonEvent start;
    XEvent ev;

    if(!(dpy = XOpenDisplay(0x0))) return 1;

    XSelectInput(dpy, DefaultRootWindow(dpy), SubstructureRedirectMask | SubstructureNotifyMask);

    int screen = DefaultScreen(dpy);
    int screen_w = DisplayWidth(dpy, screen);
    int screen_h = DisplayHeight(dpy, screen);

    start.subwindow = None;
    Window managed_windows[MAX_WINDOWS];
    int window_count = 0;
    
    adopt_current(dpy, managed_windows, &window_count);
    if (window_count > 0) draw_windows(dpy, screen_w, screen_h, managed_windows, window_count, padding);

    for(;;){
        XNextEvent(dpy, &ev);
	if(ev.type == MapRequest){
		Window new_win = ev.xmaprequest.window;
		managed_windows[window_count] = new_win;
		window_count++;
		draw_windows(dpy, screen_w, screen_h, managed_windows, window_count, padding);
		XMapWindow(dpy, new_win);
		XSync(dpy, False);
	}else if(ev.type == UnmapNotify || ev.type == DestroyNotify){
		Window dead_win = (ev.type == UnmapNotify) ? ev.xunmap.window : ev.xdestroywindow.window;
		Window filtered_windows[MAX_WINDOWS];
		int filtered_count=0;
		filter_windows(managed_windows, window_count, filtered_windows, &filtered_count, dead_win);
		if(filtered_count<window_count){
			for(int i=0; i<filtered_count; i++){
				managed_windows[i]=filtered_windows[i];
			}
			window_count=filtered_count;
			draw_windows(dpy, screen_w, screen_h, managed_windows, window_count, padding);
		}
	}
    }
}
