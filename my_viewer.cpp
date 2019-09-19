
# include "my_viewer.h"

# include <sigogl/ui_button.h>
# include <sigogl/ui_radio_button.h>
# include <sig/sn_primitive.h>
# include <sig/sn_transform.h>
# include <sig/sn_manipulator.h>

# include <sigogl/ws_run.h>

MyViewer::MyViewer ( int x, int y, int w, int h, const char* l ) : WsViewer(x,y,w,h,l)
{
	_nbut=0;
	_animating=false;
	build_ui ();
	build_scene ();
}

void MyViewer::build_ui ()
{
	UiPanel *p, *sp;
	UiManager* uim = WsWindow::uim();
	p = uim->add_panel ( "", UiPanel::HorizLeft );
	p->add ( new UiButton ( "View", sp=new UiPanel() ) );
	{	UiPanel* p=sp;
		p->add ( _nbut=new UiCheckButton ( "Normals", EvNormals ) ); 
	}
	p->add ( new UiButton ( "Animate", EvAnimate ) );
	p->add ( new UiButton ( "Exit", EvExit ) ); p->top()->separate();
}

void MyViewer::add_model ( SnShape* s, GsVec p )
{
	// This method demonstrates how to add some elements to our scene graph: lines,
	// and a shape, and all in a group under a SnManipulator.
	// Therefore we are also demonstrating the use of a manipulator to allow the user to
	// change the position of the object with the mouse. If you do not need mouse interaction,
	// you can just use a SnTransform to apply a transformation instead of a SnManipulator.
	// You would then add the transform as 1st element of the group, and set g->separator(true).
	// Your scene graph should always be carefully designed according to your application needs.

	SnManipulator* manip = new SnManipulator;
	GsMat m;
	m.translation ( p );
	manip->initial_mat ( m );

	SnGroup* g = new SnGroup;
	SnLines* l = new SnLines;
	SnTransform* t = new SnTransform;
	//m = (GsMat((float)cos(90), (float)-sin(90), 0, 0, (float)sin(90), (float)cos(90), 0, 0, 0, 0, 1, 0, 0, 0, 0, 1));
	

	l->color(GsColor::orange);
	g->add(s);
	g->add(l);
	g->add(t);
	manip->child(g);

	//t->get().roty((float)0.10472);
	t->get().translation(p);
	manip->visible(false); // call this to turn off mouse interaction

	rootg()->add(manip);
}

void MyViewer::build_scene ()
{
	SnPrimitive* p;
	SnGroup* g = new SnGroup;
	//SnTransform* tr = (SnTransform*)((SnGroup*)((SnManipulator*)rootg()->get(0))->child(g))->get(1);

	p = new SnPrimitive(GsPrimitive::Cylinder, 5, 5, 0.25f); // watch face
	p->prim().material.diffuse = GsColor::darkred;
	p->prim().orientation = GsQuat(GsVec::i, (float)GS_PIDIV2); 
	// rotate 90 degrees

	add_model(p, GsVec(0, 0, 0));

	p = new SnPrimitive(GsPrimitive::Cylinder, 0.25, 0.25, 2.5f); // hand 1
	//g->separator(true);
	//g->add(tr = new SnTransform);
	//g->add(new SnModel(*p));
	//tr->get().translation(0.5f, 0, 0);
	//rootg()->add(g);
	p->prim().material.diffuse = GsColor::red;
	// p->prim().orientation = GsQuat(GsVec::i, (float)GS_PIDIV2); 
	// rotate 90 degrees

	add_model(p, GsVec(0, 2.5f, 0.5f));

	p = new SnPrimitive(GsPrimitive::Cylinder, 0.5, 0, 0.5); // hand 1's arrow
	p->prim().material.diffuse = GsColor::red;

	add_model(p, GsVec(0, 5.5f, 0.5f));

	p = new SnPrimitive(GsPrimitive::Cylinder, 0.25, 0.25, 1); // hand 2
	p->prim().material.diffuse = GsColor::orange;

	add_model(p, GsVec(0, 1, 0.5f));

	p = new SnPrimitive(GsPrimitive::Cylinder, 0.5, 0, 0.5); // hand 2's arrow
	p->prim().material.diffuse = GsColor::orange;

	add_model(p, GsVec(0, 2.5f, 0.5f));
}

// Below is an example of how to control the main loop of an animation:
void MyViewer::run_animation ()
{
	if ( _animating ) return; // avoid recursive calls
	_animating = true;
	
	//int ind = gs_random ( 0, rootg()->size()-1 ); // pick one child
	int ind = 1;
	SnManipulator* manip = rootg()->get<SnManipulator>(ind); // access one of the manipulators
	GsMat m = manip->mat();

	double frdt = 1.0/30.0; // delta time to reach given number of frames per second
	double v = 1; // target velocity is 1 unit per second
	double t=0, lt=0, t0=gs_time();
	do // run for a while:
	{	while ( t-lt<frdt ) { ws_check(); t=gs_time()-t0; } // wait until it is time for next frame
		double yinc = (t - lt) * v;
		if ( t>1 ) yinc=-yinc; // after 1 sec: go down
		lt = t;
		m.e24 += (float)yinc;
		if ( m.e24<0 ) m.e24=0; // make sure it does not go below 0
		manip->initial_mat ( m );
		render(); // notify it needs redraw
		ws_check(); // redraw now
	}	while ( m.e24>0 );
	_animating = false;
}

void MyViewer::show_normals ( bool view )
{
	// Note that primitives are only converted to meshes in GsModel
	// at the first draw call.
	GsArray<GsVec> fn;
	SnGroup* r = (SnGroup*)root();
	for ( int k=0; k<r->size(); k++ )
	{	SnManipulator* manip = r->get<SnManipulator>(k);
		SnShape* s = manip->child<SnGroup>()->get<SnShape>(0);
		SnLines* l = manip->child<SnGroup>()->get<SnLines>(1);
		if ( !view ) { l->visible(false); continue; }
		l->visible ( true );
		if ( !l->empty() ) continue; // build only once
		l->init();
		if ( s->instance_name()==SnPrimitive::class_name )
		{	GsModel& m = *((SnModel*)s)->model();
			m.get_normals_per_face ( fn );
			const GsVec* n = fn.pt();
			float f = 0.33f;
			for ( int i=0; i<m.F.size(); i++ )
			{	const GsVec& a=m.V[m.F[i].a]; l->push ( a, a+(*n++)*f );
				const GsVec& b=m.V[m.F[i].b]; l->push ( b, b+(*n++)*f );
				const GsVec& c=m.V[m.F[i].c]; l->push ( c, c+(*n++)*f );
			}
		}  
	}
}

int MyViewer::handle_keyboard ( const GsEvent &e )
{
	int ret = WsViewer::handle_keyboard ( e ); // 1st let system check events
	if ( ret ) return ret;

	switch ( e.key )
	{	case GsEvent::KeyEsc : gs_exit(); return 1;
		case 'n' : { bool b=!_nbut->value(); _nbut->value(b); show_normals(b); return 1; }
		default: gsout<<"Key pressed: "<<e.key<<gsnl;
	}

	return 0;
}

int MyViewer::uievent ( int e )
{
	switch ( e )
	{	case EvNormals: show_normals(_nbut->value()); return 1;
		case EvAnimate: run_animation(); return 1;
		case EvExit: gs_exit();
	}
	return WsViewer::uievent(e);
}
