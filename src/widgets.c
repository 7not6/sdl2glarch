#include "common.h"

Widget widgetHead, *widgetTail;
Widget *activeWidget;

void initWidgets(void)
{
	memset(&widgetHead, 0, sizeof(Widget));
	widgetTail = &widgetHead;
}

Widget *createWidget(char *name)
{
	Widget *w;

	w = malloc(sizeof(Widget));
	memset(w, 0, sizeof(Widget));
	widgetTail->next = w;
	w->prev = widgetTail;
	widgetTail = w;

	STRNCPY(w->name, name,32);

	return w;
}

void cleanWidgets()
{
 
  	Widget *head=widgetHead.next,*currentNode;
  
  	while (head != NULL) {
    		currentNode = head;

    		head = head->next;
    		free(currentNode);
  	}
}

