//
// VhelpText.h
//
// Defines a sub-class of textEdit used to display read-only text

#include "textEdit.h"

class VhelpText : public textEdit {
	public:
		void keyboardEvent(tkKeyboardEvent *);
		void cut();
	};
