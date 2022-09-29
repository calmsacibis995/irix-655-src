#pragma once

#include "tkParentView.h"
#include "tkRenderItem.h"

class BorderPntView : public tkParentView {
protected:
	tkRenderItem*	borderModel;
public:
	BorderPntView();
	~BorderPntView();

	virtual void setBorderModel(tkRenderItem *m);
	void addView(tkView& v) { addAView(v); }

	virtual void paint();
	virtual void setBounds(Box2 const&);	// resets borderModel's bounds as well.
};
