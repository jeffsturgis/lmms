// generated by Fast Light User Interface Designer (fluid) version 1.0300

#include "ResonanceUI.h"
//Copyright (c) 2002-2005 Nasca Octavian Paul
//License: GNU GPL version 2 or later
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ResonanceGraph::ResonanceGraph(int x,int y, int w, int h, const char *label):Fl_Box(x,y,w,h,label) {
  respar=NULL;
cbwidget=NULL;
applybutton=NULL;
}

void ResonanceGraph::init(Resonance *respar_,Fl_Value_Output *khzvalue_,Fl_Value_Output *dbvalue_) {
  respar=respar_;
khzvalue=khzvalue_;
dbvalue=dbvalue_;
oldx=-1;
khzval=-1;
}

void ResonanceGraph::draw_freq_line(REALTYPE freq,int type) {
  REALTYPE freqx=respar->getfreqpos(freq);
switch(type){
  case 0:fl_line_style(FL_SOLID);break;
  case 1:fl_line_style(FL_DOT);break;
  case 2:fl_line_style(FL_DASH);break;
}; 


if ((freqx>0.0)&&(freqx<1.0))
   fl_line(x()+(int) (freqx*w()),y(),
   x()+(int) (freqx*w()),y()+h());
}

void ResonanceGraph::draw() {
  int ox=x(),oy=y(),lx=w(),ly=h(),i,ix,iy,oiy;
REALTYPE freqx;

fl_color(FL_BLACK);
fl_rectf(ox,oy,lx,ly);


//draw the lines
fl_color(FL_GRAY);

fl_line_style(FL_SOLID);
fl_line(ox+2,oy+ly/2,ox+lx-2,oy+ly/2);

freqx=respar->getfreqpos(1000.0);
if ((freqx>0.0)&&(freqx<1.0))
   fl_line(ox+(int) (freqx*lx),oy,
    ox+(int) (freqx*lx),oy+ly);

for (i=1;i<10;i++){
   if(i==1){
     draw_freq_line(i*100.0,0);
     draw_freq_line(i*1000.0,0);
   }else 
    if (i==5){
      draw_freq_line(i*100.0,2);
      draw_freq_line(i*1000.0,2);
    }else{
      draw_freq_line(i*100.0,1);
      draw_freq_line(i*1000.0,1);
    };
};

draw_freq_line(10000.0,0);
draw_freq_line(20000.0,1);

fl_line_style(FL_DOT);
int GY=10;if (ly<GY*3) GY=-1;
for (i=1;i<GY;i++){
   int tmp=(int)(ly/(REALTYPE)GY*i);
   fl_line(ox+2,oy+tmp,ox+lx-2,oy+tmp);
};



//draw the data
fl_color(FL_RED);
fl_line_style(FL_SOLID);
oiy=(int)(respar->Prespoints[0]/128.0*ly);
for (i=1;i<N_RES_POINTS;i++){
   ix=(int)(i*1.0/N_RES_POINTS*lx);
   iy=(int)(respar->Prespoints[i]/128.0*ly);
   fl_line(ox+ix-1,oy+ly-oiy,ox+ix,oy+ly-iy);
   oiy=iy;
};
}

int ResonanceGraph::handle(int event) {
  int x_=Fl::event_x()-x();
int y_=Fl::event_y()-y();
if ( (x_>=0)&&(x_<w()) && (y_>=0)&&(y_<h())){
   khzvalue->value(respar->getfreqx(x_*1.0/w())/1000.0);
   dbvalue->value((1.0-y_*2.0/h())*respar->PmaxdB);
};

if ((event==FL_PUSH)||(event==FL_DRAG)){
  int leftbutton=1;
  if (Fl::event_button()==FL_RIGHT_MOUSE) leftbutton=0;
  if (x_<0) x_=0;if (y_<0) y_=0;
  if (x_>=w()) x_=w();if (y_>=h()-1) y_=h()-1;

  if ((oldx<0)||(oldx==x_)){
    int sn=(int)(x_*1.0/w()*N_RES_POINTS);
    int sp=127-(int)(y_*1.0/h()*127);
    if (leftbutton!=0) respar->setpoint(sn,sp);
        else respar->setpoint(sn,64);
  } else {
    int x1=oldx;
    int x2=x_;
    int y1=oldy;
    int y2=y_;
    if (oldx>x_){
      x1=x_;y1=y_;
      x2=oldx;y2=oldy;
    };
   for (int i=0;i<x2-x1;i++){
      int sn=(int)((i+x1)*1.0/w()*N_RES_POINTS);
      REALTYPE yy=(y2-y1)*1.0/(x2-x1)*i;
      int sp=127-(int)((y1+yy)/h()*127);
      if (leftbutton!=0) respar->setpoint(sn,sp);
         else respar->setpoint(sn,64);
    };
  };

  oldx=x_;oldy=y_;
  redraw();
};

if (event==FL_RELEASE) {
	oldx=-1;
	if (cbwidget!=NULL) {
		cbwidget->do_callback();
		if (applybutton!=NULL) {
			applybutton->color(FL_RED);
			applybutton->redraw();

		};
	};
};

return(1);
}

void ResonanceGraph::setcbwidget(Fl_Widget *cbwidget,Fl_Widget *applybutton) {
  this->cbwidget=cbwidget;
this->applybutton=applybutton;
}

void ResonanceUI::cb_Close_i(Fl_Button*, void*) {
  resonancewindow->hide();
}
void ResonanceUI::cb_Close(Fl_Button* o, void* v) {
  ((ResonanceUI*)(o->parent()->user_data()))->cb_Close_i(o,v);
}

void ResonanceUI::cb_Zero_i(Fl_Button*, void*) {
  for (int i=0;i<N_RES_POINTS;i++) 
   respar->setpoint(i,64);
resonancewindow->redraw();
redrawPADnoteApply();
}
void ResonanceUI::cb_Zero(Fl_Button* o, void* v) {
  ((ResonanceUI*)(o->parent()->user_data()))->cb_Zero_i(o,v);
}

void ResonanceUI::cb_Smooth_i(Fl_Button*, void*) {
  respar->smooth();
resonancewindow->redraw();
redrawPADnoteApply();
}
void ResonanceUI::cb_Smooth(Fl_Button* o, void* v) {
  ((ResonanceUI*)(o->parent()->user_data()))->cb_Smooth_i(o,v);
}

void ResonanceUI::cb_enabled_i(Fl_Check_Button* o, void*) {
  respar->Penabled=(int) o->value();
redrawPADnoteApply();
}
void ResonanceUI::cb_enabled(Fl_Check_Button* o, void* v) {
  ((ResonanceUI*)(o->parent()->user_data()))->cb_enabled_i(o,v);
}

void ResonanceUI::cb_maxdb_i(Fl_Roller* o, void*) {
  maxdbvo->value(o->value());
respar->PmaxdB=(int) o->value();
redrawPADnoteApply();
}
void ResonanceUI::cb_maxdb(Fl_Roller* o, void* v) {
  ((ResonanceUI*)(o->parent()->user_data()))->cb_maxdb_i(o,v);
}

void ResonanceUI::cb_maxdbvo_i(Fl_Value_Output* o, void*) {
  o->value(respar->PmaxdB);
}
void ResonanceUI::cb_maxdbvo(Fl_Value_Output* o, void* v) {
  ((ResonanceUI*)(o->parent()->user_data()))->cb_maxdbvo_i(o,v);
}

void ResonanceUI::cb_centerfreqvo_i(Fl_Value_Output* o, void*) {
  o->value(respar->getcenterfreq()/1000.0);
}
void ResonanceUI::cb_centerfreqvo(Fl_Value_Output* o, void* v) {
  ((ResonanceUI*)(o->parent()->user_data()))->cb_centerfreqvo_i(o,v);
}

void ResonanceUI::cb_octavesfreqvo_i(Fl_Value_Output* o, void*) {
  o->value(respar->getoctavesfreq());
}
void ResonanceUI::cb_octavesfreqvo(Fl_Value_Output* o, void* v) {
  ((ResonanceUI*)(o->parent()->user_data()))->cb_octavesfreqvo_i(o,v);
}

void ResonanceUI::cb_RND2_i(Fl_Button*, void*) {
  respar->randomize(1);
resonancewindow->redraw();
redrawPADnoteApply();
}
void ResonanceUI::cb_RND2(Fl_Button* o, void* v) {
  ((ResonanceUI*)(o->parent()->user_data()))->cb_RND2_i(o,v);
}

void ResonanceUI::cb_RND1_i(Fl_Button*, void*) {
  respar->randomize(0);
resonancewindow->redraw();
redrawPADnoteApply();
}
void ResonanceUI::cb_RND1(Fl_Button* o, void* v) {
  ((ResonanceUI*)(o->parent()->user_data()))->cb_RND1_i(o,v);
}

void ResonanceUI::cb_RND3_i(Fl_Button*, void*) {
  respar->randomize(2);
resonancewindow->redraw();
redrawPADnoteApply();
}
void ResonanceUI::cb_RND3(Fl_Button* o, void* v) {
  ((ResonanceUI*)(o->parent()->user_data()))->cb_RND3_i(o,v);
}

void ResonanceUI::cb_p1st_i(Fl_Check_Button* o, void*) {
  respar->Pprotectthefundamental=(int) o->value();
redrawPADnoteApply();
}
void ResonanceUI::cb_p1st(Fl_Check_Button* o, void* v) {
  ((ResonanceUI*)(o->parent()->user_data()))->cb_p1st_i(o,v);
}

void ResonanceUI::cb_InterpP_i(Fl_Button*, void*) {
  int type;
if (Fl::event_button()==FL_LEFT_MOUSE) type=0;
       else type=1;
respar->interpolatepeaks(type);
resonancewindow->redraw();
redrawPADnoteApply();
}
void ResonanceUI::cb_InterpP(Fl_Button* o, void* v) {
  ((ResonanceUI*)(o->parent()->user_data()))->cb_InterpP_i(o,v);
}

void ResonanceUI::cb_centerfreq_i(WidgetPDial* o, void*) {
  respar->Pcenterfreq=(int)o->value();
centerfreqvo->do_callback();
rg->redraw();
redrawPADnoteApply();
}
void ResonanceUI::cb_centerfreq(WidgetPDial* o, void* v) {
  ((ResonanceUI*)(o->parent()->user_data()))->cb_centerfreq_i(o,v);
}

void ResonanceUI::cb_octavesfreq_i(WidgetPDial* o, void*) {
  respar->Poctavesfreq=(int)o->value();
octavesfreqvo->do_callback();
rg->redraw();
redrawPADnoteApply();
}
void ResonanceUI::cb_octavesfreq(WidgetPDial* o, void* v) {
  ((ResonanceUI*)(o->parent()->user_data()))->cb_octavesfreq_i(o,v);
}

void ResonanceUI::cb_C_i(Fl_Button*, void*) {
  presetsui->copy(respar);
}
void ResonanceUI::cb_C(Fl_Button* o, void* v) {
  ((ResonanceUI*)(o->parent()->user_data()))->cb_C_i(o,v);
}

void ResonanceUI::cb_P_i(Fl_Button*, void*) {
  presetsui->paste(respar,this);
}
void ResonanceUI::cb_P(Fl_Button* o, void* v) {
  ((ResonanceUI*)(o->parent()->user_data()))->cb_P_i(o,v);
}

void ResonanceUI::cb_applybutton_i(Fl_Button*, void*) {
  applybutton->color(FL_GRAY);
applybutton->redraw();
if (cbapplywidget!=NULL) {
	cbapplywidget->do_callback();
	cbapplywidget->color(FL_GRAY);
	cbapplywidget->redraw();
};
}
void ResonanceUI::cb_applybutton(Fl_Button* o, void* v) {
  ((ResonanceUI*)(o->parent()->user_data()))->cb_applybutton_i(o,v);
}

Fl_Double_Window* ResonanceUI::make_window() {
  { resonancewindow = new Fl_Double_Window(780, 305, "Resonance");
    resonancewindow->user_data((void*)(this));
    { khzvalue = new Fl_Value_Output(415, 264, 45, 18, "kHz");
      khzvalue->labelsize(12);
      khzvalue->minimum(0.001);
      khzvalue->maximum(48);
      khzvalue->step(0.01);
      khzvalue->textfont(1);
      khzvalue->textsize(12);
      khzvalue->align(Fl_Align(FL_ALIGN_RIGHT));
      //this widget must be before the calling widgets
    } // Fl_Value_Output* khzvalue
    { dbvalue = new Fl_Value_Output(415, 282, 45, 18, "dB");
      dbvalue->labelsize(12);
      dbvalue->minimum(-150);
      dbvalue->maximum(150);
      dbvalue->step(0.1);
      dbvalue->textfont(1);
      dbvalue->textsize(12);
      dbvalue->align(Fl_Align(FL_ALIGN_RIGHT));
      //this widget must be before the calling widgets
    } // Fl_Value_Output* dbvalue
    { Fl_Group* o = new Fl_Group(6, 5, 768, 256);
      o->box(FL_BORDER_BOX);
      rg=new ResonanceGraph(o->x(),o->y(),o->w(),o->h(),"");
      rg->init(respar,khzvalue,dbvalue);
      rg->show();
      o->end();
    } // Fl_Group* o
    { Fl_Button* o = new Fl_Button(690, 283, 84, 17, "Close");
      o->box(FL_THIN_UP_BOX);
      o->callback((Fl_Callback*)cb_Close);
    } // Fl_Button* o
    { Fl_Button* o = new Fl_Button(491, 264, 66, 15, "Zero");
      o->tooltip("Clear the resonance function");
      o->box(FL_THIN_UP_BOX);
      o->labelfont(1);
      o->labelsize(12);
      o->callback((Fl_Callback*)cb_Zero);
    } // Fl_Button* o
    { Fl_Button* o = new Fl_Button(491, 282, 66, 18, "Smooth");
      o->tooltip("Smooth the resonance function");
      o->box(FL_THIN_UP_BOX);
      o->labelfont(1);
      o->labelsize(12);
      o->callback((Fl_Callback*)cb_Smooth);
    } // Fl_Button* o
    { Fl_Check_Button* o = enabled = new Fl_Check_Button(6, 270, 78, 27, "Enable");
      enabled->box(FL_THIN_UP_BOX);
      enabled->down_box(FL_DOWN_BOX);
      enabled->callback((Fl_Callback*)cb_enabled);
      o->value(respar->Penabled);
    } // Fl_Check_Button* enabled
    { maxdb = new Fl_Roller(90, 282, 84, 15);
      maxdb->type(1);
      maxdb->minimum(1);
      maxdb->maximum(90);
      maxdb->step(1);
      maxdb->value(30);
      maxdb->callback((Fl_Callback*)cb_maxdb);
    } // Fl_Roller* maxdb
    { Fl_Value_Output* o = maxdbvo = new Fl_Value_Output(126, 264, 24, 18, "Max.");
      maxdbvo->tooltip("The Maximum amplitude (dB)");
      maxdbvo->labelsize(12);
      maxdbvo->minimum(1);
      maxdbvo->maximum(127);
      maxdbvo->step(1);
      maxdbvo->value(30);
      maxdbvo->textfont(1);
      maxdbvo->textsize(12);
      maxdbvo->callback((Fl_Callback*)cb_maxdbvo);
      o->value(respar->PmaxdB);
    } // Fl_Value_Output* maxdbvo
    { new Fl_Box(150, 264, 24, 18, "dB");
    } // Fl_Box* o
    { Fl_Value_Output* o = centerfreqvo = new Fl_Value_Output(210, 264, 33, 18, "C.f.");
      centerfreqvo->tooltip("Center Frequency (kHz)");
      centerfreqvo->labelsize(12);
      centerfreqvo->minimum(1);
      centerfreqvo->maximum(10);
      centerfreqvo->step(0.01);
      centerfreqvo->value(1);
      centerfreqvo->textfont(1);
      centerfreqvo->textsize(12);
      centerfreqvo->callback((Fl_Callback*)cb_centerfreqvo);
      centerfreqvo->when(3);
      o->value(respar->getcenterfreq()/1000.0);
    } // Fl_Value_Output* centerfreqvo
    { Fl_Value_Output* o = octavesfreqvo = new Fl_Value_Output(210, 282, 33, 18, "Oct.");
      octavesfreqvo->tooltip("No. of octaves");
      octavesfreqvo->labelsize(12);
      octavesfreqvo->minimum(1);
      octavesfreqvo->maximum(127);
      octavesfreqvo->step(1);
      octavesfreqvo->value(30);
      octavesfreqvo->textfont(1);
      octavesfreqvo->textsize(12);
      octavesfreqvo->callback((Fl_Callback*)cb_octavesfreqvo);
      octavesfreqvo->when(3);
      o->value(respar->getoctavesfreq());
    } // Fl_Value_Output* octavesfreqvo
    { Fl_Button* o = new Fl_Button(566, 276, 42, 12, "RND2");
      o->tooltip("Randomize the resonance function");
      o->box(FL_THIN_UP_BOX);
      o->labelfont(1);
      o->labelsize(10);
      o->callback((Fl_Callback*)cb_RND2);
    } // Fl_Button* o
    { Fl_Button* o = new Fl_Button(566, 264, 42, 12, "RND1");
      o->tooltip("Randomize the resonance function");
      o->box(FL_THIN_UP_BOX);
      o->labelfont(1);
      o->labelsize(10);
      o->callback((Fl_Callback*)cb_RND1);
    } // Fl_Button* o
    { Fl_Button* o = new Fl_Button(566, 288, 42, 12, "RND3");
      o->tooltip("Randomize the resonance function");
      o->box(FL_THIN_UP_BOX);
      o->labelfont(1);
      o->labelsize(10);
      o->callback((Fl_Callback*)cb_RND3);
    } // Fl_Button* o
    { Fl_Check_Button* o = p1st = new Fl_Check_Button(365, 285, 45, 15, "P.1st");
      p1st->tooltip("Protect the fundamental frequency (do not damp the first harmonic)");
      p1st->down_box(FL_DOWN_BOX);
      p1st->labelsize(10);
      p1st->callback((Fl_Callback*)cb_p1st);
      o->value(respar->Pprotectthefundamental);
    } // Fl_Check_Button* p1st
    { Fl_Button* o = new Fl_Button(365, 265, 46, 15, "InterpP");
      o->tooltip("Interpolate the peaks");
      o->box(FL_THIN_UP_BOX);
      o->labelfont(1);
      o->labelsize(10);
      o->callback((Fl_Callback*)cb_InterpP);
    } // Fl_Button* o
    { WidgetPDial* o = centerfreq = new WidgetPDial(245, 265, 30, 30, "C.f.");
      centerfreq->box(FL_ROUND_UP_BOX);
      centerfreq->color(FL_BACKGROUND_COLOR);
      centerfreq->selection_color(FL_INACTIVE_COLOR);
      centerfreq->labeltype(FL_NORMAL_LABEL);
      centerfreq->labelfont(0);
      centerfreq->labelsize(10);
      centerfreq->labelcolor(FL_FOREGROUND_COLOR);
      centerfreq->maximum(127);
      centerfreq->step(1);
      centerfreq->callback((Fl_Callback*)cb_centerfreq);
      centerfreq->align(Fl_Align(FL_ALIGN_BOTTOM));
      centerfreq->when(FL_WHEN_CHANGED);
      o->value(respar->Pcenterfreq);
    } // WidgetPDial* centerfreq
    { WidgetPDial* o = octavesfreq = new WidgetPDial(280, 265, 30, 30, "Oct.");
      octavesfreq->box(FL_ROUND_UP_BOX);
      octavesfreq->color(FL_BACKGROUND_COLOR);
      octavesfreq->selection_color(FL_INACTIVE_COLOR);
      octavesfreq->labeltype(FL_NORMAL_LABEL);
      octavesfreq->labelfont(0);
      octavesfreq->labelsize(10);
      octavesfreq->labelcolor(FL_FOREGROUND_COLOR);
      octavesfreq->maximum(127);
      octavesfreq->step(1);
      octavesfreq->callback((Fl_Callback*)cb_octavesfreq);
      octavesfreq->align(Fl_Align(FL_ALIGN_BOTTOM));
      octavesfreq->when(FL_WHEN_CHANGED);
      o->value(respar->Poctavesfreq);
    } // WidgetPDial* octavesfreq
    { Fl_Button* o = new Fl_Button(625, 275, 25, 15, "C");
      o->box(FL_THIN_UP_BOX);
      o->color((Fl_Color)179);
      o->labelfont(1);
      o->labelsize(11);
      o->labelcolor(FL_BACKGROUND2_COLOR);
      o->callback((Fl_Callback*)cb_C);
    } // Fl_Button* o
    { Fl_Button* o = new Fl_Button(655, 275, 25, 15, "P");
      o->box(FL_THIN_UP_BOX);
      o->color((Fl_Color)179);
      o->labelfont(1);
      o->labelsize(11);
      o->labelcolor(FL_BACKGROUND2_COLOR);
      o->callback((Fl_Callback*)cb_P);
    } // Fl_Button* o
    { applybutton = new Fl_Button(690, 265, 85, 15, "Apply");
      applybutton->box(FL_THIN_UP_BOX);
      applybutton->labelfont(1);
      applybutton->labelsize(11);
      applybutton->callback((Fl_Callback*)cb_applybutton);
    } // Fl_Button* applybutton
    resonancewindow->end();
  } // Fl_Double_Window* resonancewindow
  return resonancewindow;
}

ResonanceUI::ResonanceUI(Resonance *respar_) {
  respar=respar_;
cbwidget=NULL;
cbapplywidget=NULL;
make_window();
applybutton->hide();
}

ResonanceUI::~ResonanceUI() {
  resonancewindow->hide();
}

void ResonanceUI::redrawPADnoteApply() {
  if (cbwidget!=NULL) {
	cbwidget->do_callback();
	applybutton->color(FL_RED);
	applybutton->redraw();
};
}

void ResonanceUI::setcbwidget(Fl_Widget *cbwidget,Fl_Widget *cbapplywidget) {
  this->cbwidget=cbwidget;
this->cbapplywidget=cbapplywidget;
rg->setcbwidget(cbwidget,applybutton);
applybutton->show();
}

void ResonanceUI::refresh() {
  redrawPADnoteApply();

enabled->value(respar->Penabled);

maxdb->value(respar->PmaxdB);
maxdbvo->value(respar->PmaxdB);

centerfreqvo->value(respar->getcenterfreq()/1000.0);
octavesfreqvo->value(respar->getoctavesfreq());

centerfreq->value(respar->Pcenterfreq);
octavesfreq->value(respar->Poctavesfreq);

p1st->value(respar->Pprotectthefundamental);

rg->redraw();
}
