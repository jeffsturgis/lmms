/*
 * organic.cpp - additive synthesizer for organ-like sounds
 *
 * Copyright (c) 2006-2008 Andreas Brandmaier <andy/at/brandmaier/dot/de>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */


#include "organic.h"


#include <QtXml/QDomElement>
#include <QtGui/QPainter>


#include "engine.h"
#include "instrument_track.h"
#include "knob.h"
#include "note_play_handle.h"
#include "oscillator.h"
#include "pixmap_button.h"
#include "templates.h"
#include "tooltip.h"

#undef SINGLE_SOURCE_COMPILE
#include "embed.cpp"


extern "C"
{

plugin::descriptor PLUGIN_EXPORT organic_plugin_descriptor =
{
	STRINGIFY_PLUGIN_NAME( PLUGIN_NAME ),
	"Organic",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"Additive Synthesizer for organ-like sounds" ),
	"Andreas Brandmaier <andreas/at/brandmaier.de>",
	0x0100,
	plugin::Instrument,
	new pluginPixmapLoader( "logo" ),
	NULL
} ;

}

QPixmap * organicInstrumentView::s_artwork = NULL;


/***********************************************************************
*
*	class OrganicInstrument
*
*	lmms - plugin 
*
***********************************************************************/


organicInstrument::organicInstrument( instrumentTrack * _instrument_track ) :
	instrument( _instrument_track, &organic_plugin_descriptor ),
	m_modulationAlgo( oscillator::SignalMix ),
	m_fx1Model( 0.0f, 0.0f, 0.99f, 0.01f , this, tr( "Distortion" ) ),
	m_volModel( 100.0f, 0.0f, 200.0f, 1.0f, this, tr( "Volume" ) )
{
	m_numOscillators = 8;

	m_osc = new oscillatorObject*[ m_numOscillators ];
	for (int i=0; i < m_numOscillators; i++)
	{
		m_osc[i] = new oscillatorObject( this, i );
		m_osc[i]->m_numOscillators = m_numOscillators;

		// Connect events 
		connect( &m_osc[i]->m_oscModel, SIGNAL( dataChanged() ),
				m_osc[i], SLOT ( oscButtonChanged() ) );
		connect( &m_osc[i]->m_volModel, SIGNAL( dataChanged() ),
				m_osc[i], SLOT( updateVolume() ) );
		connect( &m_osc[i]->m_panModel, SIGNAL( dataChanged() ),
				m_osc[i], SLOT( updateVolume() ) );
		connect( &m_osc[i]->m_detuneModel, SIGNAL( dataChanged() ),
				m_osc[i], SLOT( updateDetuning() ) );

		m_osc[i]->updateVolume();

	}

	m_osc[0]->m_harmonic = log2f( 0.5f );	// one octave below
	m_osc[1]->m_harmonic = log2f( 0.75f );	// a fifth below
	m_osc[2]->m_harmonic = log2f( 1.0f );	// base freq
	m_osc[3]->m_harmonic = log2f( 2.0f );	// first overtone
	m_osc[4]->m_harmonic = log2f( 3.0f );	// second overtone
	m_osc[5]->m_harmonic = log2f( 4.0f );	// .
	m_osc[6]->m_harmonic = log2f( 5.0f );	// .
	m_osc[7]->m_harmonic = log2f( 6.0f );	// .

	for (int i=0; i < m_numOscillators; i++) {
		m_osc[i]->updateVolume();
		m_osc[i]->updateDetuning();
	}
	

	connect( engine::getMixer(), SIGNAL( sampleRateChanged() ),
					this, SLOT( updateAllDetuning() ) );

}




organicInstrument::~organicInstrument()
{
	delete[] m_osc;
}




void organicInstrument::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setAttribute( "num_osc", QString::number( m_numOscillators ) );
	m_fx1Model.saveSettings( _doc, _this, "foldback" );
	m_volModel.saveSettings( _doc, _this, "vol" );

	for( int i = 0; i < m_numOscillators; ++i )
	{
		QString is = QString::number( i );
		m_osc[i]->m_volModel.saveSettings( _doc, _this, "vol" + is );
		m_osc[i]->m_panModel.saveSettings( _doc, _this, "pan" + is );
		_this.setAttribute( "harmonic" + is, QString::number(
					powf( 2.0f, m_osc[i]->m_harmonic ) ) );
		m_osc[i]->m_detuneModel.saveSettings( _doc, _this, "detune"
									+ is );
		m_osc[i]->m_oscModel.saveSettings( _doc, _this, "wavetype"
									+ is );
	}
}




void organicInstrument::loadSettings( const QDomElement & _this )
{
//	m_numOscillators =  _this.attribute( "num_osc" ).
	//							toInt();

	for( int i = 0; i < m_numOscillators; ++i )
	{
		QString is = QString::number( i );
		m_osc[i]->m_volModel.loadSettings( _this, "vol" + is );
		m_osc[i]->m_detuneModel.loadSettings( _this, "detune" + is );
		m_osc[i]->m_panModel.loadSettings( _this, "pan" + is );
		m_osc[i]->m_oscModel.loadSettings( _this, "wavetype" + is );
	}
	
	m_volModel.loadSettings( _this, "vol" );
	m_fx1Model.loadSettings( _this, "foldback" );
}


QString organicInstrument::nodeName( void ) const
{
	return( organic_plugin_descriptor.name );
}




void organicInstrument::playNote( notePlayHandle * _n, bool,
						sampleFrame * _working_buffer )
{
	if( _n->totalFramesPlayed() == 0 || _n->m_pluginData == NULL )
	{
		oscillator * oscs_l[m_numOscillators];
		oscillator * oscs_r[m_numOscillators];

		for( Sint8 i = m_numOscillators - 1; i >= 0; --i )
		{
			
			m_osc[i]->m_phaseOffsetLeft = rand()
							/ ( RAND_MAX + 1.0f );
			m_osc[i]->m_phaseOffsetRight = rand()
							/ ( RAND_MAX + 1.0f );
			

			
			// initialise ocillators
			
			if( i == m_numOscillators - 1 )
			{
				// create left oscillator
				oscs_l[i] = new oscillator(
						m_osc[i]->m_waveShape,
						m_modulationAlgo,
						_n->frequency(),
						m_osc[i]->m_detuningLeft,
						m_osc[i]->m_phaseOffsetLeft,
						m_osc[i]->m_volumeLeft );
				// create right oscillator
				oscs_r[i] = new oscillator(
						m_osc[i]->m_waveShape,
						m_modulationAlgo,
						_n->frequency(),
						m_osc[i]->m_detuningRight,
						m_osc[i]->m_phaseOffsetRight,
						m_osc[i]->m_volumeRight );
			}
			else
			{
				// create left oscillator
				oscs_l[i] = new oscillator(
						m_osc[i]->m_waveShape,
						m_modulationAlgo,
						_n->frequency(),
						m_osc[i]->m_detuningLeft,
						m_osc[i]->m_phaseOffsetLeft,
						m_osc[i]->m_volumeLeft,
						oscs_l[i + 1] );
				// create right oscillator
				oscs_r[i] = new oscillator(
						m_osc[i]->m_waveShape,
						m_modulationAlgo,
						_n->frequency(),
						m_osc[i]->m_detuningRight,
						m_osc[i]->m_phaseOffsetRight,
						m_osc[i]->m_volumeRight,
						oscs_r[i + 1] );
			}
			
				
		}

		_n->m_pluginData = new oscPtr;
		static_cast<oscPtr *>( _n->m_pluginData )->oscLeft = oscs_l[0];
		static_cast<oscPtr *>( _n->m_pluginData )->oscRight = oscs_r[0];
	}

	oscillator * osc_l = static_cast<oscPtr *>( _n->m_pluginData )->oscLeft;
	oscillator * osc_r = static_cast<oscPtr *>( _n->m_pluginData
								)->oscRight;

	const fpp_t frames = _n->framesLeftForCurrentPeriod();

	osc_l->update( _working_buffer, frames, 0 );
	osc_r->update( _working_buffer, frames, 1 );


	// -- fx section --
	
	// fxKnob is [0;1]
	float t =  m_fx1Model.value();
	
	for (int i=0 ; i < frames ; i++)
	{
		_working_buffer[i][0] = waveshape( _working_buffer[i][0], t ) *
						m_volModel.value() / 100.0f;
		_working_buffer[i][1] = waveshape( _working_buffer[i][1], t ) *
						m_volModel.value() / 100.0f;
	}
	
	// -- --

	getInstrumentTrack()->processAudioBuffer( _working_buffer, frames, _n );
}




void organicInstrument::deleteNotePluginData( notePlayHandle * _n )
{
	delete static_cast<oscillator *>( static_cast<oscPtr *>(
						_n->m_pluginData )->oscLeft );
	delete static_cast<oscillator *>( static_cast<oscPtr *>(
						_n->m_pluginData )->oscRight );
	delete static_cast<oscPtr *>( _n->m_pluginData );
}

/*float inline organicInstrument::foldback(float in, float threshold)
{
  if (in>threshold || in<-threshold)
  {
    in= fabs(fabs(fmod(in - threshold, threshold*4)) - threshold*2) - threshold;
  }
  return in;
}
*/




float inline organicInstrument::waveshape(float in, float amount)
{
	float k = 2.0f * amount / ( 1.0f - amount );

	return( ( 1.0f + k ) * in / ( 1.0f + k * fabs( in ) ) );
}




void organicInstrument::randomiseSettings( void )
{

	for( int i = 0; i < m_numOscillators; i++ )
	{
		m_osc[i]->m_volModel.setValue( intRand( 0, 100 ) );

		m_osc[i]->m_detuneModel.setValue( intRand( -5, 5 ) );

		m_osc[i]->m_panModel.setValue( 0 );

		m_osc[i]->m_oscModel.setValue( intRand( 0, 5 ) );
	}

}




void organicInstrument::updateAllDetuning( void )
{
	for( int i = 0; i < m_numOscillators; ++i )
	{
		m_osc[i]->updateDetuning();
	}
}




int organicInstrument::intRand( int min, int max )
{
//	int randn = min+int((max-min)*rand()/(RAND_MAX + 1.0));	
//	cout << randn << endl;
	int randn = ( rand() % (max - min) ) + min;
	return( randn );
}


pluginView * organicInstrument::instantiateView( QWidget * _parent )
{
	return( new organicInstrumentView( this, _parent ) );
}




class organicKnob : public knob
{
public:
	organicKnob( QWidget * _parent ) :
		knob( knobStyled, _parent )
	{
		setFixedSize( 21, 21 );
	}
};




organicInstrumentView::organicInstrumentView( instrument * _instrument,
							QWidget * _parent ) :
	instrumentView( _instrument, _parent )
{
	organicInstrument * oi = castModel<organicInstrument>();

	setAutoFillBackground( TRUE );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap(
								"artwork" ) );
	setPalette( pal );

	// setup knob for FX1
	m_fx1Knob = new organicKnob( this );
	m_fx1Knob->move( 15, 201 );
	m_fx1Knob->setFixedSize( 37, 47 );
	m_fx1Knob->setHintText( tr( "Distortion:" ) + " ", "%" );
	m_fx1Knob->setObjectName( "fx1Knob" );

	// setup volume-knob
	m_volKnob = new organicKnob( this );
	m_volKnob->setVolumeKnob( TRUE );
	m_volKnob->move( 60, 201 );
	m_volKnob->setFixedSize( 37, 47 );
	m_volKnob->setHintText( tr( "Volume:" ).arg( 1 ) + " ", "%" );
	m_volKnob->setObjectName( "volKnob" );

	// randomise
	m_randBtn = new pixmapButton( this, tr( "Randomise" ) );
	m_randBtn->move( 148, 224 );
	m_randBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"randomise_pressed" ) );
	m_randBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
								"randomise" ) );

	connect( m_randBtn, SIGNAL ( clicked() ),
					oi, SLOT( randomiseSettings() ) );


	if( s_artwork == NULL )
	{
		s_artwork = new QPixmap( PLUGIN_NAME::getIconPixmap(
								"artwork" ) );
	}

}


organicInstrumentView::~organicInstrumentView()
{
	delete[] m_oscKnobs;
}


void organicInstrumentView::modelChanged( void )
{
	organicInstrument * oi = castModel<organicInstrument>();
	const float y=91.3;
	const float rowHeight = 26.52f;
	const float x=53.4;
	const float colWidth = 23.829f; // 54.4 77.2 220.2

	m_numOscillators = oi->m_numOscillators;
	
	m_fx1Knob->setModel( &oi->m_fx1Model );
	m_volKnob->setModel( &oi->m_volModel );

	// TODO: Delete existing oscKnobs if they exist
	
	m_oscKnobs = new oscillatorKnobs[ m_numOscillators ];

	// Create knobs, now that we know how many to make
	for( int i = 0; i < m_numOscillators; ++i )
	{
		// setup waveform-knob
		knob * oscKnob = new organicKnob( this );
		oscKnob->move( x + i * colWidth, y );
		oscKnob->setHintText( tr( "Osc %1 waveform:" ).arg(
					i + 1 ) + " ", "%" );
										
		// setup volume-knob
		knob * volKnob = new knob( knobStyled, this );
		volKnob->setVolumeKnob( TRUE );
		volKnob->move( x + i * colWidth, y + rowHeight*1 );
		volKnob->setFixedSize( 21, 21 );
		volKnob->setHintText( tr( "Osc %1 volume:" ).arg(
							i + 1 ) + " ", "%" );
							
		// setup panning-knob
		knob * panKnob = new organicKnob( this );
		panKnob->move( x + i  * colWidth, y + rowHeight*2 );
		panKnob->setHintText( tr("Osc %1 panning:").arg(
							i + 1 ) + " ", "" );
							
		// setup knob for left fine-detuning
		knob * detuneKnob = new organicKnob( this );
		detuneKnob->move( x + i * colWidth, y + rowHeight*3 );
		detuneKnob->setHintText( tr( "Osc %1 fine detuning "
							"left:" ).arg( i + 1 )
							+ " ", " " +
							tr( "cents" ) );

		m_oscKnobs[i] = oscillatorKnobs( volKnob, oscKnob, panKnob, detuneKnob );

		// Attach to models
		m_oscKnobs[i].m_volKnob->setModel(
					&oi->m_osc[i]->m_volModel );
		m_oscKnobs[i].m_oscKnob->setModel(
					&oi->m_osc[i]->m_oscModel );
		m_oscKnobs[i].m_panKnob->setModel(
					&oi->m_osc[i]->m_panModel );
		m_oscKnobs[i].m_detuneKnob->setModel(
					&oi->m_osc[i]->m_detuneModel );
	}
}




oscillatorObject::oscillatorObject( model * _parent, int _index ) :
	model( _parent ),
	m_waveShape( oscillator::SineWave, 0, oscillator::NumWaveShapes-1, this ),
	m_oscModel( 0.0f, 0.0f, 5.0f, 1.0f,
			this, tr( "Osc %1 waveform" ).arg( _index + 1 ) ),
	m_volModel( 100.0f, 0.0f, 100.0f, 1.0f,
			this, tr( "Osc %1 volume" ).arg( _index + 1 ) ),
	m_panModel( DefaultPanning, PanningLeft, PanningRight, 1.0f,
			this, tr( "Osc %1 panning" ).arg( _index + 1 ) ),
	m_detuneModel( 0.0f, -100.0f, 100.0f, 1.0f, 
			this, tr( "Osc %1 fine detuning left" ).arg( _index + 1 ) )
{
}




oscillatorObject::~oscillatorObject()
{
}




void oscillatorObject::oscButtonChanged( void )
{

	static oscillator::WaveShapes shapes[] =
	{
		oscillator::SineWave,
		oscillator::SawWave,
		oscillator::SquareWave,
		oscillator::TriangleWave,
		oscillator::MoogSawWave,
		oscillator::ExponentialWave
	} ;

	m_waveShape.setValue( shapes[(int)roundf( m_oscModel.value() )] );

}




void oscillatorObject::updateVolume( void )
{
	m_volumeLeft = ( 1.0f - m_panModel.value() / (float)PanningRight )
			* m_volModel.value() / m_numOscillators / 100.0f;
	m_volumeRight = ( 1.0f + m_panModel.value() / (float)PanningRight )
			* m_volModel.value() / m_numOscillators / 100.0f;
}




void oscillatorObject::updateDetuning( void )
{
	m_detuningLeft = powf( 2.0f, m_harmonic
				+ (float)m_detuneModel.value() / 100.0f ) /
				engine::getMixer()->processingSampleRate();
	m_detuningRight = powf( 2.0f, m_harmonic
				- (float)m_detuneModel.value() / 100.0f ) /
				engine::getMixer()->processingSampleRate();
}




extern "C"
{

// neccessary for getting instance out of shared lib
plugin * PLUGIN_EXPORT lmms_plugin_main( model *, void * _data )
{
	return( new organicInstrument( static_cast<instrumentTrack *>( _data ) ) );
}


}

/*
 * some notes & ideas for the future of this plugin:
 * 
 * - 32.692 Hz in the bass to 5919.85 Hz of treble in  a Hammond organ
 * => implement harmonic foldback
 * 
 m_osc[i].m_oscModel->setInitValue( 0.0f );
 * - randomize preset 
 */



#include "organic.moc"
