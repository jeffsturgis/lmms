#ifndef SINGLE_SOURCE_COMPILE

/*
 * track.cpp - implementation of classes concerning tracks -> neccessary for
 *             all track-like objects (beat/bassline, sample-track...)
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

/** \file track.cpp
 *  \brief All classes concerning tracks and track-like objects
 */

/*
 * \mainpage Track classes
 *
 * \section introduction Introduction
 * 
 * \todo fill this out
 */

#include "track.h"

#include <assert.h>

#include <QtGui/QLayout>
#include <QtGui/QMenu>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QStyleOption>


#include "automation_pattern.h"
#include "automation_track.h"
#include "bb_editor.h"
#include "bb_track.h"
#include "bb_track_container.h"
#include "clipboard.h"
#include "embed.h"
#include "engine.h"
#include "gui_templates.h"
#include "instrument_track.h"
#include "main_window.h"
#include "mmp.h"
#include "pixmap_button.h"
#include "project_journal.h"
#include "sample_track.h"
#include "song.h"
#include "string_pair_drag.h"
#include "templates.h"
#include "text_float.h"
#include "tooltip.h"
#include "track_container.h"


/*! The width of the resize grip in pixels
 */
const Sint16 RESIZE_GRIP_WIDTH = 4;

/*! The size of the track buttons in pixels
 */
const Uint16 TRACK_OP_BTN_WIDTH = 20;
const Uint16 TRACK_OP_BTN_HEIGHT = 14;

/*! The minimum track height in pixels
 *
 * Tracks can be resized by shift-dragging anywhere inside the track
 * display.  This sets the minimum size in pixels for a track.
 */
const Uint16 MINIMAL_TRACK_HEIGHT = 32;


/*! A pointer for that text bubble used when moving segments, etc.
 *
 * In a number of situations, LMMS displays a floating text bubble
 * beside the cursor as you move or resize elements of a track about.
 * This pointer keeps track of it, as you only ever need one at a time.
 */
textFloat * trackContentObjectView::s_textFloat = NULL;


// ===========================================================================
// trackContentObject
// ===========================================================================
/*! \brief Create a new trackContentObject
 *
 *  Creates a new track content object for the given track.
 *
 * \param _track The track that will contain the new object
 */
trackContentObject::trackContentObject( track * _track ) :
	model( _track ),
	m_track( _track ),
	m_startPosition(),
	m_length(),
	m_mutedModel( FALSE, this )
{
	m_track->addTCO( this );
	setJournalling( FALSE );
	movePosition( 0 );
	changeLength( 0 );
	setJournalling( TRUE );
}




/*! \brief Destroy a trackContentObject
 *
 *  Destroys the given track content object.
 *
 */
trackContentObject::~trackContentObject()
{
/*! \brief Start a drag event on this track View.
 *
 *  \param _dee the DragEnterEvent to start.
 */
	emit destroyed();

	m_track->removeTCO( this );
}




/*! \brief Move this trackContentObject's position in time
 *
 *  If the track content object has moved, update its position.  We
 *  also add a journal entry for undo and update the display.
 *
 * \param _pos The new position of the track content object.
 */
void trackContentObject::movePosition( const midiTime & _pos )
{
	if( m_startPosition != _pos )
	{
		addJournalEntry( journalEntry( Move, m_startPosition - _pos ) );
		m_startPosition = _pos;
		engine::getSong()->updateLength();
	}
	emit positionChanged();
}




/*! \brief Change the length of this trackContentObject
 *
 *  If the track content object's length has chaanged, update it.  We
 *  also add a journal entry for undo and update the display.
 *
 * \param _length The new length of the track content object.
 */
void trackContentObject::changeLength( const midiTime & _length )
{
	if( m_length != _length )
	{
		addJournalEntry( journalEntry( Resize, m_length - _length ) );
		m_length = _length;
		engine::getSong()->updateLength();
	}
	emit lengthChanged();
}




/*! \brief Undo one journal entry of this trackContentObject
 *
 *  Restore the previous state of this track content object.  This will
 *  restore the position or the length of the track content object
 *  depending on what was changed.
 *
 * \param _je The journal entry to undo
 */
void trackContentObject::undoStep( journalEntry & _je )
{
	saveJournallingState( FALSE );
	switch( _je.actionID() )
	{
		case Move:
			movePosition( startPosition() + _je.data().toInt() );
			break;
		case Resize:
			changeLength( length() + _je.data().toInt() );
			break;
	}
	restoreJournallingState();
}




/*! \brief Redo one journal entry of this trackContentObject
 *
 *  Undoes one 'undo' of this track content object.
 *
 * \param _je The journal entry to redo
 */
void trackContentObject::redoStep( journalEntry & _je )
{
	journalEntry je( _je.actionID(), -_je.data().toInt() );
	undoStep( je );
}




/*! \brief Cut this trackContentObject from its track to the clipboard.
 *
 *  Perform the 'cut' action of the clipboard - copies the track content
 *  object to the clipboard and then removes it from the track.
 */
void trackContentObject::cut( void )
{
	copy();
	deleteLater();
}




/*! \brief Copy this trackContentObject to the clipboard.
 *
 *  Copies this track content object to the clipboard.
 */
void trackContentObject::copy( void )
{
	clipboard::copy( this );
}




/*! \brief Pastes this trackContentObject into a track.
 *
 *  Pastes this track content object into a track.
 *
 * \param _je The journal entry to undo
 */
void trackContentObject::paste( void )
{
	if( clipboard::getContent( nodeName() ) != NULL )
	{
		restoreState( *( clipboard::getContent( nodeName() ) ) );
	}
}




/*! \brief Mutes this trackContentObject
 *
 *  Restore the previous state of this track content object.  This will
 *  restore the position or the length of the track content object
 *  depending on what was changed.
 *
 * \param _je The journal entry to undo
 */
void trackContentObject::toggleMute( void )
{
	m_mutedModel.setValue( !m_mutedModel.value() );
	emit dataChanged();
}







// ===========================================================================
// trackContentObjectView
// ===========================================================================
/*! \brief Create a new trackContentObjectView
 *
 *  Creates a new track content object view for the given
 *  track content object in the given track view.
 *
 * \param _tco The track content object to be displayed
 * \param _tv  The track view that will contain the new object
 */
trackContentObjectView::trackContentObjectView( trackContentObject * _tco,
							trackView * _tv ) :
	selectableObject( _tv->getTrackContentWidget() ),
	modelView( NULL, this ),
	m_tco( _tco ),
	m_trackView( _tv ),
	m_action( NoAction ),
	m_autoResize( FALSE ),
	m_initialMouseX( 0 ),
	m_hint( NULL )
{
	if( s_textFloat == NULL )
	{
		s_textFloat = new textFloat;
		s_textFloat->setPixmap( embed::getIconPixmap( "clock" ) );
	}

	setAttribute( Qt::WA_DeleteOnClose );
	setFocusPolicy( Qt::StrongFocus );
	move( 0, 1 );
	show();

	setFixedHeight( _tv->getTrackContentWidget()->height() - 2 );
	setAcceptDrops( TRUE );
	setMouseTracking( TRUE );

	connect( m_tco, SIGNAL( lengthChanged() ),
			this, SLOT( updateLength() ) );
	connect( m_tco, SIGNAL( positionChanged() ),
			this, SLOT( updatePosition() ) );
	connect( m_tco, SIGNAL( destroyed() ), this, SLOT( close() ) );
	setModel( m_tco );

	m_trackView->getTrackContentWidget()->addTCOView( this );
}




/*! \brief Destroy a trackContentObjectView
 *
 *  Destroys the given track content object view.
 *
 */
trackContentObjectView::~trackContentObjectView()
{
	delete m_hint;
	// we have to give our track-container the focus because otherwise the
	// op-buttons of our track-widgets could become focus and when the user
	// presses space for playing song, just one of these buttons is pressed
	// which results in unwanted effects
	m_trackView->getTrackContainerView()->setFocus();
}




/*! \brief Does this trackContentObjectView have a fixed TCO?
 *
 *  Returns whether the containing trackView has fixed
 *  TCOs.
 *
 * \todo What the hell is a TCO here - track content object?  And in
 *  what circumstance are they fixed?
 */
bool trackContentObjectView::fixedTCOs( void )
{
	return( m_trackView->getTrackContainerView()->fixedTCOs() );
}




/*! \brief Close a trackContentObjectView
 *
 *  Closes a track content object view by asking the track
 *  view to remove us and then asking the QWidget to close us.
 *
 * \return Boolean state of whether the QWidget was able to close.
 */
bool trackContentObjectView::close( void )
{
	m_trackView->getTrackContentWidget()->removeTCOView( this );
	return( QWidget::close() );
}




/*! \brief Removes a trackContentObjectView from its track view.
 *
 *  Like the close() method, this asks the track view to remove this
 *  track content object view.  However, the track content object is
 *  scheduled for later deletion rather than closed immediately.
 *
 */
void trackContentObjectView::remove( void )
{
	// delete ourself
	close();
	m_tco->deleteLater();
}




/*! \brief Updates a trackContentObjectView's length
 *
 *  If this track content object view has a fixed TCO, then we must
 *  keep the width of our parent.  Otherwise, calculate our width from
 *  the track content object's length in pixels adding in the border.
 *
 */
void trackContentObjectView::updateLength( void )
{
	if( fixedTCOs() )
	{
		setFixedWidth( parentWidget()->width() );
	}
	else
	{
		setFixedWidth(
		static_cast<int>( m_tco->length() * pixelsPerTact() /
					midiTime::ticksPerTact() ) +
						TCO_BORDER_WIDTH * 2-1 );
	}
	m_trackView->getTrackContainerView()->update();
}




/*! \brief Updates a trackContentObjectView's position.
 *
 *  Ask our track view to change our position.  Then make sure that the
 *  track view is updated in case this position has changed the track
 *  view's length.
 *
 */
void trackContentObjectView::updatePosition( void )
{
	m_trackView->getTrackContentWidget()->changePosition();
	// moving a TCO can result in change of song-length etc.,
	// therefore we update the track-container
	m_trackView->getTrackContainerView()->update();
}



/*! \brief Change the trackContentObjectView's display when something
 *  being dragged enters it.
 *
 *  We need to notify Qt to change our display if something being
 *  dragged has entered our 'airspace'.
 *
 * \param _dee The QDragEnterEvent to watch.
 */
void trackContentObjectView::dragEnterEvent( QDragEnterEvent * _dee )
{
	stringPairDrag::processDragEnterEvent( _dee, "tco_" +
				QString::number( m_tco->getTrack()->type() ) );
}




/*! \brief Handle something being dropped on this trackContentObjectView.
 *
 *  When something has been dropped on this trackContentObjectView, and
 *  it's a track content object, then use an instance of our mmp reader
 *  to take the xml of the track content object and turn it into something
 *  we can write over our current state.
 *
 * \param _de The QDropEvent to handle.
 */
void trackContentObjectView::dropEvent( QDropEvent * _de )
{
	QString type = stringPairDrag::decodeKey( _de );
	QString value = stringPairDrag::decodeValue( _de );
	if( type == ( "tco_" + QString::number( m_tco->getTrack()->type() ) ) )
	{
		// value contains our XML-data so simply create a
		// multimediaProject which does the rest for us...
		multimediaProject mmp( value, FALSE );
		// at least save position before getting to moved to somewhere
		// the user doesn't expect...
		midiTime pos = m_tco->startPosition();
		m_tco->restoreState( mmp.content().firstChild().toElement() );
		m_tco->movePosition( pos );
		_de->accept();
	}
}




/*! \brief Handle a dragged selection leaving our 'airspace'.
 *
 * \param _e The QEvent to watch.
 */
void trackContentObjectView::leaveEvent( QEvent * _e )
{
	while( QApplication::overrideCursor() != NULL )
	{
		QApplication::restoreOverrideCursor();
	}
	if( _e != NULL )
	{
		QWidget::leaveEvent( _e );
	}
}




/*! \brief Handle a mouse press on this trackContentObjectView.
 *
 *  Handles the various ways in which a trackContentObjectView can be
 *  used with a click of a mouse button.
 *
 *  * If our container supports rubber band selection then handle
 *    selection events.
 *  * or if shift-left button, add this object to the selection
 *  * or if ctrl-left button, start a drag-copy event
 *  * or if just plain left button, resize if we're resizeable
 *  * or if ctrl-middle button, mute the track content object
 *  * or if middle button, maybe delete the track content object.
 *
 * \param _me The QMouseEvent to handle.
 */
void trackContentObjectView::mousePressEvent( QMouseEvent * _me )
{
	if( m_trackView->getTrackContainerView()->allowRubberband() == TRUE &&
					_me->button() == Qt::LeftButton )
	{
		// if rubberband is active, we can be selected
		if( !m_trackView->getTrackContainerView()->rubberBandActive() )
		{
			if( engine::getMainWindow()->isCtrlPressed() == TRUE )
			{
				setSelected( !isSelected() );
			}
			else if( isSelected() == TRUE )
			{
				m_action = MoveSelection;
				m_initialMouseX = _me->x();
			}
		}
		else
		{
			selectableObject::mousePressEvent( _me );
		}
		return;
	}
	else if( engine::getMainWindow()->isShiftPressed() == TRUE )
	{
		// add/remove object to/from selection
		selectableObject::mousePressEvent( _me );
	}
	else if( _me->button() == Qt::LeftButton &&
			engine::getMainWindow()->isCtrlPressed() == TRUE )
	{
		// start drag-action
		multimediaProject mmp( multimediaProject::DragNDropData );
		m_tco->saveState( mmp, mmp.content() );
		QPixmap thumbnail = QPixmap::grabWidget( this ).scaled(
						128, 128,
						Qt::KeepAspectRatio,
						Qt::SmoothTransformation );
		new stringPairDrag( QString( "tco_%1" ).arg(
						m_tco->getTrack()->type() ),
					mmp.toString(), thumbnail, this );
	}
	else if( _me->button() == Qt::LeftButton &&
		/*	engine::getMainWindow()->isShiftPressed() == FALSE &&*/
							fixedTCOs() == FALSE )
	{
		// move or resize
		m_tco->setJournalling( FALSE );

		m_initialMouseX = _me->x();

		if( _me->x() < width() - RESIZE_GRIP_WIDTH )
		{
			m_action = Move;
			m_oldTime = m_tco->startPosition();
			QCursor c( Qt::SizeAllCursor );
			QApplication::setOverrideCursor( c );
			s_textFloat->setTitle( tr( "Current position" ) );
			delete m_hint;
			m_hint = textFloat::displayMessage( tr( "Hint" ),
					tr( "Press <Ctrl> and drag to make "
							"a copy." ),
					embed::getIconPixmap( "hint" ), 0 );
		}
		else if( m_autoResize == FALSE )
		{
			m_action = Resize;
			m_oldTime = m_tco->length();
			QCursor c( Qt::SizeHorCursor );
			QApplication::setOverrideCursor( c );
			s_textFloat->setTitle( tr( "Current length" ) );
			delete m_hint;
			m_hint = textFloat::displayMessage( tr( "Hint" ),
					tr( "Press <Ctrl> for free "
							"resizing." ),
					embed::getIconPixmap( "hint" ), 0 );
		}
//		s_textFloat->reparent( this );
		// setup text-float as if TCO was already moved/resized
		mouseMoveEvent( _me );
		s_textFloat->show();
	}
	else if( _me->button() == Qt::MidButton )
	{
		if( engine::getMainWindow()->isCtrlPressed() )
		{
			m_tco->toggleMute();
		}
		else if( fixedTCOs() == FALSE )
		{
			remove();
		}
	}
}




/*! \brief Handle a mouse movement (drag) on this trackContentObjectView.
 *
 *  Handles the various ways in which a trackContentObjectView can be
 *  used with a mouse drag.
 *
 *  * If in move mode, move ourselves in the track,
 *  * or if in move-selection mode, move the entire selection,
 *  * or if in resize mode, resize ourselves,
 *  * otherwise ??? 
 *
 * \param _me The QMouseEvent to handle.
 * \todo what does the final else case do here?
 */
void trackContentObjectView::mouseMoveEvent( QMouseEvent * _me )
{
	if( engine::getMainWindow()->isCtrlPressed() == TRUE )
	{
		delete m_hint;
		m_hint = NULL;
	}

	const float ppt = m_trackView->getTrackContainerView()->pixelsPerTact();
	if( m_action == Move )
	{
		const int x = mapToParent( _me->pos() ).x() - m_initialMouseX;
		midiTime t = tMax( 0, (int)
			m_trackView->getTrackContainerView()->currentPosition()+
				static_cast<int>( x * midiTime::ticksPerTact() /
									ppt ) );
		if( engine::getMainWindow()->isCtrlPressed() ==
					FALSE && _me->button() == Qt::NoButton )
		{
			t = t.toNearestTact();
		}
		m_tco->movePosition( t );
		m_trackView->getTrackContentWidget()->changePosition();
		s_textFloat->setText( QString( "%1:%2" ).
				arg( m_tco->startPosition().getTact() + 1 ).
				arg( m_tco->startPosition().getTicks() %
						midiTime::ticksPerTact() ) );
		s_textFloat->moveGlobal( this, QPoint( width() + 2,
		                                        height() + 2 ) );
	}
	else if( m_action == MoveSelection )
	{
		const int dx = _me->x() - m_initialMouseX;
		QVector<selectableObject *> so =
			m_trackView->getTrackContainerView()->selectedObjects();
		QVector<trackContentObject *> tcos;
		midiTime smallest_pos;
		// find out smallest position of all selected objects for not
		// moving an object before zero
		for( QVector<selectableObject *>::iterator it = so.begin();
							it != so.end(); ++it )
		{
			trackContentObjectView * tcov =
				dynamic_cast<trackContentObjectView *>( *it );
			if( tcov == NULL )
			{
				continue;
			}
			trackContentObject * tco = tcov->m_tco;
			tcos.push_back( tco );
			smallest_pos = tMin<int>( smallest_pos,
					(int)tco->startPosition() +
				static_cast<int>( dx *
					midiTime::ticksPerTact() / ppt ) );
		}
		for( QVector<trackContentObject *>::iterator it = tcos.begin();
							it != tcos.end(); ++it )
		{
			( *it )->movePosition( ( *it )->startPosition() +
				static_cast<int>( dx *
					midiTime::ticksPerTact() / ppt ) -
								smallest_pos );
		}
	}
	else if( m_action == Resize )
	{
		midiTime t = tMax( midiTime::ticksPerTact(),
				static_cast<int>( _me->x() *
					midiTime::ticksPerTact() / ppt ) );
		if( engine::getMainWindow()->isCtrlPressed() ==
					FALSE && _me->button() == Qt::NoButton )
		{
			t = t.toNearestTact();
		}
		m_tco->changeLength( t );
		s_textFloat->setText( tr( "%1:%2 (%3:%4 to %5:%6)" ).
				arg( m_tco->length().getTact() ).
				arg( m_tco->length().getTicks() %
						midiTime::ticksPerTact() ).
				arg( m_tco->startPosition().getTact() + 1 ).
				arg( m_tco->startPosition().getTicks() %
						midiTime::ticksPerTact() ).
				arg( m_tco->endPosition().getTact() + 1 ).
				arg( m_tco->endPosition().getTicks() %
						midiTime::ticksPerTact() ) );
		s_textFloat->moveGlobal( this, QPoint( width() + 2,
					height() + 2) );
	}
	else
	{
		if( _me->x() > width() - RESIZE_GRIP_WIDTH )
		{
			if( QApplication::overrideCursor() != NULL &&
				QApplication::overrideCursor()->shape() !=
							Qt::SizeHorCursor )
			{
				while( QApplication::overrideCursor() != NULL )
				{
					QApplication::restoreOverrideCursor();
				}
			}
			QCursor c( Qt::SizeHorCursor );
			QApplication::setOverrideCursor( c );
		}
		else
		{
			leaveEvent( NULL );
		}
	}
}




/*! \brief Handle a mouse release on this trackContentObjectView.
 *
 *  If we're in move or resize mode, journal the change as appropriate.
 *  Then tidy up.
 *
 * \param _me The QMouseEvent to handle.
 */
void trackContentObjectView::mouseReleaseEvent( QMouseEvent * _me )
{
	if( m_action == Move || m_action == Resize )
	{
		m_tco->setJournalling( TRUE );
		m_tco->addJournalEntry( journalEntry( m_action, m_oldTime -
			( ( m_action == Move ) ?
				m_tco->startPosition() : m_tco->length() ) ) );
	}
	m_action = NoAction;
	delete m_hint;
	m_hint = NULL;
	s_textFloat->hide();
	leaveEvent( NULL );
	selectableObject::mouseReleaseEvent( _me );
}




/*! \brief Set up the context menu for this trackContentObjectView.
 *
 *  Set up the various context menu events that can apply to a
 *  track content object view.
 *
 * \param _cme The QContextMenuEvent to add the actions to.
 */
void trackContentObjectView::contextMenuEvent( QContextMenuEvent * _cme )
{
	QMenu contextMenu( this );
	if( fixedTCOs() == FALSE )
	{
		contextMenu.addAction( embed::getIconPixmap( "cancel" ),
					tr( "Delete (middle mousebutton)" ),
						this, SLOT( remove() ) );
		contextMenu.addSeparator();
		contextMenu.addAction( embed::getIconPixmap( "edit_cut" ),
					tr( "Cut" ), m_tco, SLOT( cut() ) );
	}
	contextMenu.addAction( embed::getIconPixmap( "edit_copy" ),
					tr( "Copy" ), m_tco, SLOT( copy() ) );
	contextMenu.addAction( embed::getIconPixmap( "edit_paste" ),
					tr( "Paste" ), m_tco, SLOT( paste() ) );
	contextMenu.addSeparator();
	contextMenu.addAction( embed::getIconPixmap( "muted" ),
				tr( "Mute/unmute (<Ctrl> + middle click)" ),
						m_tco, SLOT( toggleMute() ) );
	constructContextMenu( &contextMenu );

	contextMenu.exec( QCursor::pos() );
}





/*! \brief How many pixels a tact (bar) takes for this trackContentObjectView.
 *
 * \return the number of pixels per tact (bar).
 */
float trackContentObjectView::pixelsPerTact( void )
{
	return( m_trackView->getTrackContainerView()->pixelsPerTact() );
}




/*! \brief Set whether this trackContentObjectView can resize.
 *
 * \param _e The boolean state of whether this track content object view
 *  is allowed to resize.
 */
void trackContentObjectView::setAutoResizeEnabled( bool _e )
{
	m_autoResize = _e;
}




// ===========================================================================
// trackContentWidget
// ===========================================================================
/*! \brief Create a new trackContentWidget
 *
 *  Creates a new track content widget for the given track.
 *  The content widget comprises the 'grip bar' and the 'tools' button
 *  for the track's context menu.
 *
 * \param _track The parent track.
 */
trackContentWidget::trackContentWidget( trackView * _parent ) :
	QWidget( _parent ),
	m_trackView( _parent )
{
	setAcceptDrops( TRUE );

	connect( _parent->getTrackContainerView(),
			SIGNAL( positionChanged( const midiTime & ) ),
			this, SLOT( changePosition( const midiTime & ) ) );

	setAutoFillBackground( false );
	setAttribute( Qt::WA_OpaquePaintEvent );
}




/*! \brief Destroy this trackContentWidget
 *
 *  Destroys the trackContentWidget.
 */
trackContentWidget::~trackContentWidget()
{
}




/*! \brief Adds a trackContentObjectView to this widget.
 *
 *  Adds a(nother) trackContentObjectView to our list of views.  We also
 *  check that our position is up-to-date.
 *
 * \param _tcov The trackContentObjectView to add.
 */
void trackContentWidget::addTCOView( trackContentObjectView * _tcov )
{
	trackContentObject * tco = _tcov->getTrackContentObject();
	QMap<QString, QVariant> map;
	map["id"] = tco->id();
	addJournalEntry( journalEntry( AddTrackContentObject, map ) );

	m_tcoViews.push_back( _tcov );

	tco->saveJournallingState( FALSE );
	changePosition();
	tco->restoreJournallingState();

}




/*! \brief Removes the given trackContentObjectView to this widget.
 *
 *  Removes the given trackContentObjectView from our list of views.
 *
 * \param _tcov The trackContentObjectView to add.
 */
void trackContentWidget::removeTCOView( trackContentObjectView * _tcov )
{
	tcoViewVector::iterator it = qFind( m_tcoViews.begin(),
						m_tcoViews.end(),
						_tcov );
	if( it != m_tcoViews.end() )
	{
		QMap<QString, QVariant> map;
		multimediaProject mmp( multimediaProject::JournalData );
		_tcov->getTrackContentObject()->saveState( mmp, mmp.content() );
		map["id"] = _tcov->getTrackContentObject()->id();
		map["state"] = mmp.toString();
		addJournalEntry( journalEntry( RemoveTrackContentObject,
								map ) );

		m_tcoViews.erase( it );
		engine::getSong()->setModified();
	}
}




/*! \brief Update ourselves by updating all the tCOViews attached.
 *
 */
void trackContentWidget::update( void )
{
	for( tcoViewVector::iterator it = m_tcoViews.begin();
				it != m_tcoViews.end(); ++it )
	{
		( *it )->setFixedHeight( height() - 2 );
		( *it )->update();
	}
	QWidget::update();
}




// resposible for moving track-content-widgets to appropriate position after
// change of visible viewport
/*! \brief Move the trackContentWidget to a new place in time
 *
 * \param _new_pos The MIDI time to move to.
 */
void trackContentWidget::changePosition( const midiTime & _new_pos )
{
	if( m_trackView->getTrackContainerView() == engine::getBBEditor() )
	{
		const int cur_bb = engine::getBBTrackContainer()->currentBB();
		setUpdatesEnabled( false );

		// first show TCO for current BB...
		for( tcoViewVector::iterator it = m_tcoViews.begin();
                    					it != m_tcoViews.end(); ++it )
		{
            if( ( *it )->getTrackContentObject()->
                            startPosition().getTact() == cur_bb )
			{
				( *it )->move( 0, ( *it )->y() );
				( *it )->raise();
				( *it )->show();
			}
			else
			{
				( *it )->lower();
			}
		}
		// ...then hide others to avoid flickering
		for( tcoViewVector::iterator it = m_tcoViews.begin();
					it != m_tcoViews.end(); ++it )
		{
            if( ( *it )->getTrackContentObject()->
                            startPosition().getTact() != cur_bb )
			{
				( *it )->hide();
			}
		}
		setUpdatesEnabled( true );
		return;
	}

	midiTime pos = _new_pos;
	if( pos < 0 )
	{
		pos = m_trackView->getTrackContainerView()->currentPosition();
	}

	const int begin = pos;
	const int end = endPosition( pos );
	const float ppt = m_trackView->getTrackContainerView()->pixelsPerTact();

	setUpdatesEnabled( false );
	for( tcoViewVector::iterator it = m_tcoViews.begin();
						it != m_tcoViews.end(); ++it )
	{
		trackContentObjectView * tcov = *it;
		trackContentObject * tco = tcov->getTrackContentObject();

		tco->changeLength( tco->length() );

		const int ts = tco->startPosition();
		const int te = tco->endPosition()-3;
		if( ( ts >= begin && ts <= end ) ||
			( te >= begin && te <= end ) ||
			( ts <= begin && te >= end ) )
		{
			tcov->move( static_cast<int>( ( ts - begin ) * ppt /
						midiTime::ticksPerTact() ),
								tcov->y() );
			if( !tcov->isVisible() )
			{
				tcov->show();
			}
		}
		else
		{
			tcov->move( -tcov->width()-10, tcov->y() );
		}
	}
	setUpdatesEnabled( true );

	// redraw background
//	update();
}




/*! \brief Respond to a drag enter event on the trackContentWidget
 *
 * \param _dee the Drag Enter Event to respond to
 */
void trackContentWidget::dragEnterEvent( QDragEnterEvent * _dee )
{
	stringPairDrag::processDragEnterEvent( _dee, "tco_" +
					QString::number( getTrack()->type() ) );
}




/*! \brief Respond to a drop event on the trackContentWidget
 *
 * \param _de the Drop Event to respond to
 */
void trackContentWidget::dropEvent( QDropEvent * _de )
{
	QString type = stringPairDrag::decodeKey( _de );
	QString value = stringPairDrag::decodeValue( _de );
	if( type == ( "tco_" + QString::number( getTrack()->type() ) ) &&
		m_trackView->getTrackContainerView()->fixedTCOs() == FALSE )
	{
		const midiTime pos = getPosition( _de->pos().x()
							).toNearestTact();
		trackContentObject * tco = getTrack()->createTCO( pos );

		// value contains our XML-data so simply create a
		// multimediaProject which does the rest for us...
		multimediaProject mmp( value, FALSE );
		// at least save position before getting moved to somewhere
		// the user doesn't expect...
		tco->restoreState( mmp.content().firstChild().toElement() );
		tco->movePosition( pos );


		_de->accept();
	}
}






/*! \brief Respond to a mouse press on the trackContentWidget
 *
 * \param _me the mouse press event to respond to
 */
void trackContentWidget::mousePressEvent( QMouseEvent * _me )
{
	if( m_trackView->getTrackContainerView()->allowRubberband() == TRUE )
	{
		QWidget::mousePressEvent( _me );
	}
	else if( engine::getMainWindow()->isShiftPressed() == TRUE )
	{
		QWidget::mousePressEvent( _me );
	}
	else if( _me->button() == Qt::LeftButton &&
			!m_trackView->getTrackContainerView()->fixedTCOs() )
	{
		const midiTime pos = getPosition( _me->x() ).getTact() *
						midiTime::ticksPerTact();
		trackContentObject * tco = getTrack()->createTCO( pos );

		tco->saveJournallingState( FALSE );
		tco->movePosition( pos );
		tco->restoreJournallingState();

	}
}




/*! \brief Repaint the trackContentWidget on command
 *
 * \param _pe the Paint Event to respond to
 */
void trackContentWidget::paintEvent( QPaintEvent * _pe )
{
	static QPixmap backgrnd;
	static int last_geometry = 0;

	QPainter p( this );
	const int tactsPerBar = 4;
	const trackContainerView * tcv = m_trackView->getTrackContainerView();

	// Assume even-pixels-per-tact. Makes sense, should be like this anyways
	int ppt = static_cast<int>( tcv->pixelsPerTact() );

	// Update background if needed
	if( ppt*height() != last_geometry )
	{
		int w = ppt * tactsPerBar;
		int h = height();
		backgrnd = QPixmap( w * 2, height() );
		QPainter pmp( &backgrnd );
		//pmp.setRenderHint( QPainter::Antialiasing );
		
		QLinearGradient grad( 0,1, 0,h-2 );
		pmp.fillRect( 0, 0, w, h, QColor(128, 128, 128) );
		grad.setColorAt( 0.0, QColor( 64, 64, 64 ) );
		grad.setColorAt( 0.3, QColor( 128, 128, 128 ) );
		grad.setColorAt( 0.5, QColor( 128, 128, 128 ) );
		grad.setColorAt( 0.95, QColor( 160, 160, 160 ) );
		//grad.setColorAt( 1.0, QColor( 128, 128, 128 ) );
		//grad.setColorAt( 1.0, QColor( 64, 64, 64) );
		pmp.fillRect( 0, 1, w, h-2, grad );

		QLinearGradient grad2( 0,1, 0, h-2 );
		pmp.fillRect( w, 0, w , h, QColor(96, 96, 96) );
		grad2.setColorAt( 0.0, QColor( 48, 48, 48 ) );
		grad2.setColorAt( 0.3, QColor( 96, 96, 96 ) );
		grad2.setColorAt( 0.5, QColor( 96, 96, 96 ) );
		grad2.setColorAt( 0.95, QColor( 120, 120, 120 ) );
		//grad2.setColorAt( 1.0, QColor( 96, 96, 96 ) );
		//grad2.setColorAt( 1.0, QColor( 48, 48, 48 ) );
		pmp.fillRect( w, 1, w , h-2, grad2 );
		
		// draw vertical lines
		//pmp.setPen( QPen( QBrush( QColor( 80, 84, 96, 192 ) ), 1 ) );
		pmp.setPen( QPen( QBrush( QColor( 0,0,0, 112 ) ), 1 ) );
		for( float x = 0.5; x < w * 2; x += ppt )
		{
			pmp.drawLine( QLineF( x, 1.0, x, h-2.0 ) );
		}
		//pmp.setPen( QPen( QColor( 255,0,0, 128 ), 1 ) );
		pmp.drawLine( 0, 1, w*2, 1 );

		pmp.setPen( QPen( QBrush( QColor( 255,255,255, 32 ) ), 1 ) );
		for( float x = 1.5; x < w * 2; x += ppt )
		{
			pmp.drawLine( QLineF( x, 1.0, x, h-2.0 ) );
		}
		//pmp.setPen( QPen( QColor( 0,255,0, 128 ), 1 ) );
		pmp.drawLine( 0, h-2, w*2, h-2 );

		pmp.end();

		last_geometry = ppt*h;
	}
		
	// Don't draw background on BB-Editor
	if( m_trackView->getTrackContainerView() != engine::getBBEditor() )
	{
		p.drawTiledPixmap( rect(), backgrnd, QPoint( 
				tcv->currentPosition().getTact() * ppt, 0 ) );
	}

}




/*! \brief Resize the trackContentWidget
 *
 * \param _re the resize event to respond to
 */
void trackContentWidget::resizeEvent( QResizeEvent * _re )
{
	update();
}




/*! \brief Undo an action on the trackContentWidget
 *
 * \param _je the details of the edit journal
 */
void trackContentWidget::undoStep( journalEntry & _je )
{
	saveJournallingState( FALSE );
	switch( _je.actionID() )
	{
		case AddTrackContentObject:
		{
			QMap<QString, QVariant> map = _je.data().toMap();
			trackContentObject * tco =
				dynamic_cast<trackContentObject *>(
			engine::getProjectJournal()->getJournallingObject(
							map["id"].toInt() ) );
			multimediaProject mmp(
					multimediaProject::JournalData );
			tco->saveState( mmp, mmp.content() );
			map["state"] = mmp.toString();
			_je.data() = map;
			tco->deleteLater();
			break;
		}

		case RemoveTrackContentObject:
		{
			trackContentObject * tco = getTrack()->createTCO(
								midiTime( 0 ) );
			multimediaProject mmp(
				_je.data().toMap()["state"].toString(), FALSE );
			tco->restoreState(
				mmp.content().firstChild().toElement() );
			break;
		}
	}
	restoreJournallingState();
}




/*! \brief Redo an action of the trackContentWidget
 *
 * \param _je the entry in the edit journal to redo.
 */
void trackContentWidget::redoStep( journalEntry & _je )
{
	switch( _je.actionID() )
	{
		case AddTrackContentObject:
		case RemoveTrackContentObject:
			_je.actionID() = ( _je.actionID() ==
						AddTrackContentObject ) ?
				RemoveTrackContentObject :
						AddTrackContentObject;
			undoStep( _je );
			_je.actionID() = ( _je.actionID() ==
						AddTrackContentObject ) ?
				RemoveTrackContentObject :
						AddTrackContentObject;
			break;
	}
}




/*! \brief Return the track shown by the trackContentWidget
 *
 */
track * trackContentWidget::getTrack( void )
{
	return( m_trackView->getTrack() );
}




/*! \brief Return the position of the trackContentWidget in Tacts.
 *
 * \param _mouse_x the mouse's current X position in pixels.
 */
midiTime trackContentWidget::getPosition( int _mouse_x )
{
	return( midiTime( m_trackView->getTrackContainerView()->
					currentPosition() + _mouse_x *
						midiTime::ticksPerTact() /
			static_cast<int>( m_trackView->
				getTrackContainerView()->pixelsPerTact() ) ) );
}



/*! \brief Return the end position of the trackContentWidget in Tacts.
 *
 * \param _pos_start the starting position of the Widget (from getPosition())
 */
midiTime trackContentWidget::endPosition( const midiTime & _pos_start )
{
	const float ppt = m_trackView->getTrackContainerView()->pixelsPerTact();
	const int w = width();
	return( _pos_start + static_cast<int>( w * midiTime::ticksPerTact() /
									ppt ) );
}






// ===========================================================================
// trackOperationsWidget
// ===========================================================================


QPixmap * trackOperationsWidget::s_grip = NULL;     /*!< grip pixmap */
QPixmap * trackOperationsWidget::s_muteOffDisabled; /*!< Mute off and disabled pixmap */
QPixmap * trackOperationsWidget::s_muteOffEnabled;  /*!< Mute off but enabled pixmap */
QPixmap * trackOperationsWidget::s_muteOnDisabled;  /*!< Mute on but disabled pixmap */
QPixmap * trackOperationsWidget::s_muteOnEnabled;   /*!< Mute on and enabled pixmap */


/*! \brief Create a new trackOperationsWidget
 *
 * The trackOperationsWidget is the grip and the mute button of a track.
 *
 * \param _parent the trackView to contain this widget
 */
trackOperationsWidget::trackOperationsWidget( trackView * _parent ) :
	QWidget( _parent ),             /*!< The parent widget */
	m_trackView( _parent )          /*!< The parent track view */
{
	if( s_grip == NULL )
	{
		s_grip = new QPixmap( embed::getIconPixmap(
							"track_op_grip" ) );
/*		s_muteOffDisabled = new QPixmap( embed::getIconPixmap(
							"mute_off_disabled" ) );
		s_muteOffEnabled = new QPixmap( embed::getIconPixmap(
							"mute_off" ) );
		s_muteOnDisabled = new QPixmap( embed::getIconPixmap(
							"mute_on_disabled" ) );
		s_muteOnEnabled = new QPixmap( embed::getIconPixmap(
							"mute_on" ) );*/
	}

	toolTip::add( this, tr( "Press <Ctrl> while clicking on move-grip "
				"to begin a new drag'n'drop-action." ) );

	QMenu * to_menu = new QMenu( this );
	to_menu->setFont( pointSize<9>( to_menu->font() ) );
	connect( to_menu, SIGNAL( aboutToShow() ), this, SLOT( updateMenu() ) );


	setObjectName( "automationEnabled" );


	m_trackOps = new QPushButton( this );
	m_trackOps->move( 12, 1 );
	m_trackOps->setMenu( to_menu );
	toolTip::add( m_trackOps, tr( "Actions for this track" ) );


	m_muteBtn = new pixmapButton( this, tr( "Mute" ) );
	m_muteBtn->setActiveGraphic( embed::getIconPixmap( "led_off" ) );
	m_muteBtn->setInactiveGraphic( embed::getIconPixmap( "led_green" ) );
	m_muteBtn->setCheckable( TRUE );
	m_muteBtn->move( 46, 8 );
	m_muteBtn->show();
	m_muteBtn->setWhatsThis(
		tr( "With this switch you can either mute this track or mute "
			"all other tracks.\nBy clicking left, this track is "
			"muted. This is useful, if you only want to listen to "
			"the other tracks without changing this track "
			"and loosing information.\nWhen you click right on "
			"this switch, all other tracks will be "
			"muted. This is useful, if you only want to listen to "
			"this track." ) );
	toolTip::add( m_muteBtn, tr( "Mute this track" ) );

	m_soloBtn = new pixmapButton( this, tr( "Mute" ) );
	m_soloBtn->setActiveGraphic( embed::getIconPixmap( "led_red" ) );
	m_soloBtn->setInactiveGraphic( embed::getIconPixmap( "led_off" ) );
	m_soloBtn->setCheckable( TRUE );
	m_soloBtn->move( 62, 8 );
	m_soloBtn->show();
	toolTip::add( m_soloBtn, tr( "Solo" ) );

	connect( this, SIGNAL( trackRemovalScheduled( trackView * ) ),
			m_trackView->getTrackContainerView(),
				SLOT( deleteTrackView( trackView * ) ),
							Qt::QueuedConnection );
			
}




/*! \brief Destroy an existing trackOperationsWidget
 *
 */
trackOperationsWidget::~trackOperationsWidget()
{
}




/*! \brief Respond to trackOperationsWidget mouse events
 *
 *  If it's the left mouse button, and Ctrl is held down, and we're
 *  not a Beat+Bassline Editor track, then start a new drag event to
 *  copy this track.
 *
 *  Otherwise, ignore all other events.
 *
 *  \param _me The mouse event to respond to.
 */
void trackOperationsWidget::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton &&
		engine::getMainWindow()->isCtrlPressed() == TRUE &&
			m_trackView->getTrack()->type() != track::BBTrack )
	{
		multimediaProject mmp( multimediaProject::DragNDropData );
		m_trackView->getTrack()->saveState( mmp, mmp.content() );
		new stringPairDrag( QString( "track_%1" ).arg(
					m_trackView->getTrack()->type() ),
			mmp.toString(), QPixmap::grabWidget(
				m_trackView->getTrackSettingsWidget() ),
									this );
	}
	else if( _me->button() == Qt::LeftButton )
	{
		// track-widget (parent-widget) initiates track-move
		_me->ignore();
	}
}





/*! \brief Repaint the trackOperationsWidget
 *
 *  If we're not moving, and in the Beat+Bassline Editor, then turn
 *  automation on or off depending on its previous state and show
 *  ourselves.
 *
 *  Otherwise, hide ourselves.
 *
 *  \todo Flesh this out a bit - is it correct?
 *  \param _pe The paint event to respond to
 */
void trackOperationsWidget::paintEvent( QPaintEvent * _pe )
{
	QPainter p( this );
	p.fillRect( rect(), QColor( 56, 60, 72 ) );

	if( m_trackView->isMovingTrack() == FALSE )
	{
		p.drawPixmap( 2, 2, *s_grip );
		m_trackOps->show();
		m_muteBtn->show();
	}
	else
	{
		m_trackOps->hide();
		m_muteBtn->hide();
	}
}





/*! \brief Clone this track
 *
 */
void trackOperationsWidget::cloneTrack( void )
{
	engine::getMixer()->lock();
	m_trackView->getTrack()->clone();
	engine::getMixer()->unlock();
}




/*! \brief Remove this track from the track list
 *
 */
void trackOperationsWidget::removeTrack( void )
{
	emit trackRemovalScheduled( m_trackView );
}




/*! \brief Update the trackOperationsWidget context menu
 *
 *  If we're in the Beat+Bassline Editor, we can supply the enable or
 *  disable automation options.  For all track types, we have the Clone
 *  and Remove options as well.
 */
void trackOperationsWidget::updateMenu( void )
{
	QMenu * to_menu = m_trackOps->menu();
	to_menu->clear();
	to_menu->addAction( embed::getIconPixmap( "edit_copy", 16, 16 ),
						tr( "Clone this track" ),
						this, SLOT( cloneTrack() ) );
	to_menu->addAction( embed::getIconPixmap( "cancel", 16, 16 ),
						tr( "Remove this track" ),
						this, SLOT( removeTrack() ) );
}





// ===========================================================================
// track
// ===========================================================================

/*! \brief Create a new (empty) track object
 *
 *  The track object is the whole track, linking its contents, its
 *  automation, name, type, and so forth.
 *
 * \param _type The type of track (Song Editor or Beat+Bassline Editor)
 * \param _tc The track Container object to encapsulate in this track.
 *
 * \todo check the definitions of all the properties - are they OK?
 */
track::track( TrackTypes _type, trackContainer * _tc ) :
	model( _tc ),                   /*!< The track model */
	m_trackContainer( _tc ),        /*!< The track container object */
	m_type( _type ),                /*!< The track type */
	m_name(),                       /*!< The track's name */
	m_pixmapLoader( NULL ),         /*!< For loading the track's pixmaps */
	m_mutedModel( FALSE, this ),    /*!< For controlling track muting */
	m_soloModel( FALSE, this ),     /*!< For controlling track soloing */
	m_trackContentObjects()         /*!< The track content objects (segments) */
{
	m_trackContainer->addTrack( this );
}




/*! \brief Destroy this track
 *
 *  If the track container is a Beat+Bassline container, step through
 *  its list of tracks and remove us.
 *
 *  Then delete the trackContentObject's contents, remove this track from
 *  the track container.
 *
 *  Finally step through this track's automation and forget all of them.
 */
track::~track()
{
	while( !m_trackContentObjects.isEmpty() )
	{
		delete m_trackContentObjects.last();
	}

	m_trackContainer->removeTrack( this );
}




/*! \brief Create a track based on the given track type and container.
 *
 *  \param _tt The type of track to create
 *  \param _tc The track container to attach to
 */
track * track::create( TrackTypes _tt, trackContainer * _tc )
{
	track * t = NULL;

	switch( _tt )
	{
		case InstrumentTrack: t = new instrumentTrack( _tc ); break;
		case BBTrack: t = new bbTrack( _tc ); break;
//		case SampleTrack: t = new sampleTrack( _tc ); break;
//		case EVENT_TRACK:
//		case VIDEO_TRACK:
		case AutomationTrack: t = new automationTrack( _tc ); break;
		case HiddenAutomationTrack:
				t = new automationTrack( _tc, TRUE ); break;
		default: break;
	}

	_tc->updateAfterTrackAdd();

	return( t );
}




/*! \brief Create a track from track type in a QDomElement and a container object.
 *
 *  \param _this The QDomElement containing the type of track to create
 *  \param _tc The track container to attach to
 */
track * track::create( const QDomElement & _this, trackContainer * _tc )
{
	track * t = create(
		static_cast<TrackTypes>( _this.attribute( "type" ).toInt() ),
									_tc );
	if( t != NULL )
	{
		t->restoreState( _this );
	}
	return( t );
}




/*! \brief Clone a track from this track
 *
 */
void track::clone( void )
{
	QDomDocument doc;
	QDomElement parent = doc.createElement( "clone" );
	saveState( doc, parent );
	create( parent.firstChild().toElement(), m_trackContainer );
}






/*! \brief Save this track's settings to file
 *
 *  We save the track type and its muted state, then append the track-
 *  specific settings.  Then we iterate through the trackContentObjects
 *  and save all their states in turn.
 *
 *  \param _doc The QDomDocument to use to save
 *  \param _this The The QDomElement to save into
 *  \todo Does this accurately describe the parameters?  I think not!?
 *  \todo Save the track height
 */
void track::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setTagName( "track" );
	_this.setAttribute( "type", type() );
	_this.setAttribute( "muted", isMuted() );
// ### TODO
//	_this.setAttribute( "height", m_trackView->height() );

	QDomElement ts_de = _doc.createElement( nodeName() );
	// let actual track (instrumentTrack, bbTrack, sampleTrack etc.) save
	// its settings
	_this.appendChild( ts_de );
	saveTrackSpecificSettings( _doc, ts_de );

	// now save settings of all TCO's
	for( tcoVector::const_iterator it = m_trackContentObjects.begin();
				it != m_trackContentObjects.end(); ++it )
	{
		( *it )->saveState( _doc, _this );
	}
}




/*! \brief Load the settings from a file
 *
 *  We load the track's type and muted state, then clear out our
 *  current trackContentObject.
 *
 *  Then we step through the QDomElement's children and load the
 *  track-specific settings and trackContentObjects states from it
 *  one at a time.
 *
 *  \param _this the QDomElement to load track settings from
 *  \todo Load the track height.
 */
void track::loadSettings( const QDomElement & _this )
{
	if( _this.attribute( "type" ).toInt() != type() )
	{
		qWarning( "Current track-type does not match track-type of "
							"settings-node!\n" );
	}

	setMuted( _this.attribute( "muted" ).toInt() );

	while( !m_trackContentObjects.empty() )
	{
		delete m_trackContentObjects.front();
		m_trackContentObjects.erase( m_trackContentObjects.begin() );
	}

	QDomNode node = _this.firstChild();
	while( !node.isNull() )
	{
		if( node.isElement() )
		{
			if( node.nodeName() == nodeName() )
			{
				loadTrackSpecificSettings( node.toElement() );
			}
			else if(
			!node.toElement().attribute( "metadata" ).toInt() )
			{
				trackContentObject * tco = createTCO(
								midiTime( 0 ) );
				tco->restoreState( node.toElement() );
				saveJournallingState( FALSE );
//				addTCO( tco );
				restoreJournallingState();
			}
		}
		node = node.nextSibling();
        }
/*
	if( _this.attribute( "height" ).toInt() >= MINIMAL_TRACK_HEIGHT )
	{
		m_trackView->setFixedHeight(
					_this.attribute( "height" ).toInt() );
	}*/
}




/*! \brief Add another trackContentObject into this track
 *
 *  \param _tco The trackContentObject to attach to this track.
 */
trackContentObject * track::addTCO( trackContentObject * _tco )
{
	m_trackContentObjects.push_back( _tco );

//	emit dataChanged();
	emit trackContentObjectAdded( _tco );

	//engine::getSongEditor()->setModified();
	return( _tco );		// just for convenience
}




/*! \brief Remove a given trackContentObject from this track
 *
 *  \param _tco The trackContentObject to remove from this track.
 */
void track::removeTCO( trackContentObject * _tco )
{
	tcoVector::iterator it = qFind( m_trackContentObjects.begin(),
					m_trackContentObjects.end(),
					_tco );
	if( it != m_trackContentObjects.end() )
	{
		m_trackContentObjects.erase( it );
		engine::getSong()->setModified();
	}
}




/*! \brief Return the number of trackContentObjects we contain
 *
 *  \return the number of trackContentObjects we currently contain.
 */
int track::numOfTCOs( void )
{
	return( m_trackContentObjects.size() );
}




/*! \brief Get a trackContentObject by number
 *
 *  If the TCO number is less than our TCO array size then fetch that
 *  numbered object from the array.  Otherwise we warn the user that
 *  we've somehow requested a TCO that is too large, and create a new
 *  TCO for them.
 *  \param _tco_number The number of the trackContentObject to fetch.
 *  \return the given trackContentObject or a new one if out of range.
 *  \todo reject TCO numbers less than zero.
 *  \todo if we create a TCO here, should we somehow attach it to the
 *     track?
 */
trackContentObject * track::getTCO( int _tco_num )
{
	if( _tco_num < m_trackContentObjects.size() )
	{
		return( m_trackContentObjects[_tco_num] );
	}
	printf( "called track::getTCO( %d ), "
			"but TCO %d doesn't exist\n", _tco_num, _tco_num );
	return( createTCO( _tco_num * midiTime::ticksPerTact() ) );
	
}




/*! \brief Determine the given trackContentObject's number in our array.
 *
 *  \param _tco The trackContentObject to search for.
 *  \return its number in our array.
 */
int track::getTCONum( trackContentObject * _tco )
{
//	for( int i = 0; i < getTrackContentWidget()->numOfTCOs(); ++i )
	tcoVector::iterator it = qFind( m_trackContentObjects.begin(),
					m_trackContentObjects.end(),
					_tco );
	if( it != m_trackContentObjects.end() )
	{
/*		if( getTCO( i ) == _tco )
		{
			return( i );
		}*/
		return( it - m_trackContentObjects.begin() );
	}
	qWarning( "track::getTCONum(...) -> _tco not found!\n" );
	return( 0 );
}




/*! \brief Retrieve a list of trackContentObjects that fall within a period.
 *
 *  Here we're interested in a range of trackContentObjects that fall
 *  completely within a given time period - their start must be no earlier
 *  than the given start time and their end must be no later than the given
 *  end time.
 *
 *  We return the TCOs we find in order by time, earliest TCOs first.
 *
 *  \param _tco_c The list to contain the found trackContentObjects.
 *  \param _start The MIDI start time of the range.
 *  \param _end   The MIDI endi time of the range.
 */
void track::getTCOsInRange( tcoVector & _tco_v, const midiTime & _start,
							const midiTime & _end )
{
	for( tcoVector::iterator it_o = m_trackContentObjects.begin();
				it_o != m_trackContentObjects.end(); ++it_o )
	{
		trackContentObject * tco = ( *it_o );
		int s = tco->startPosition();
		int e = tco->endPosition();
		if( ( s <= _end ) && ( e >= _start ) )
		{
			// ok, TCO is posated within given range
			// now let's search according position for TCO in list
			// 	-> list is ordered by TCO's position afterwards
			bool inserted = FALSE;
			for( tcoVector::iterator it = _tco_v.begin();
						it != _tco_v.end(); ++it )
			{
				if( ( *it )->startPosition() >= s )
				{
					_tco_v.insert( it, tco );
					inserted = TRUE;
					break;
				}
			}
			if( inserted == FALSE )
			{
				// no TCOs found posated behind current TCO...
				_tco_v.push_back( tco );
			}
		}
	}
}




/*! \brief Swap the position of two trackContentObjects.
 *
 *  First, we arrange to swap the positions of the two TCOs in the
 *  trackContentObjects list.  Then we swap their start times as well.
 *
 *  \param _tco_num1 The first trackContentObject to swap.
 *  \param _tco_num2 The second trackContentObject to swap.
 */
void track::swapPositionOfTCOs( int _tco_num1, int _tco_num2 )
{
	// TODO: range-checking
	qSwap( m_trackContentObjects[_tco_num1],
					m_trackContentObjects[_tco_num2] );

	const midiTime pos = m_trackContentObjects[_tco_num1]->startPosition();

	m_trackContentObjects[_tco_num1]->movePosition(
			m_trackContentObjects[_tco_num2]->startPosition() );
	m_trackContentObjects[_tco_num2]->movePosition( pos );
}




/*! \brief Move all the trackContentObjects after a certain time later by one bar.
 *
 *  \param _pos The time at which we want to insert the bar.
 *  \todo if we stepped through this list last to first, and the list was
 *    in ascending order by TCO time, once we hit a TCO that was earlier
 *    than the insert time, we could fall out of the loop early.
 */
void track::insertTact( const midiTime & _pos )
{
	// we'll increase the position of every TCO, positioned behind _pos, by
	// one tact
	for( tcoVector::iterator it = m_trackContentObjects.begin();
				it != m_trackContentObjects.end(); ++it )
	{
		if( ( *it )->startPosition() >= _pos )
		{
			( *it )->movePosition( (*it)->startPosition() +
						midiTime::ticksPerTact() );
		}
	}
}




/*! \brief Move all the trackContentObjects after a certain time earlier by one bar.
 *
 *  \param _pos The time at which we want to remove the bar.
 */
void track::removeTact( const midiTime & _pos )
{
	// we'll decrease the position of every TCO, positioned behind _pos, by
	// one tact
	for( tcoVector::iterator it = m_trackContentObjects.begin();
				it != m_trackContentObjects.end(); ++it )
	{
		if( ( *it )->startPosition() >= _pos )
		{
			( *it )->movePosition( tMax( ( *it )->startPosition() -
						midiTime::ticksPerTact(), 0 ) );
		}
	}
}




/*! \brief Return the length of the entire track in bars
 *
 *  We step through our list of TCOs and determine their end position,
 *  keeping track of the latest time found in ticks.  Then we return
 *  that in bars by dividing by the number of ticks per bar.
 */
tact track::length( void ) const
{
	// find last end-position
	tick last = 0;
	for( tcoVector::const_iterator it = m_trackContentObjects.begin();
				it != m_trackContentObjects.end(); ++it )
	{
		const tick cur = ( *it )->endPosition();
		if( cur > last )
		{
			last = cur;
		}
	}

	return( last / midiTime::ticksPerTact() );
}



/*! \brief Invert the track's solo state.
 *
 *  We have to go through all the tracks determining if any other track
 *  is already soloed.  Then we have to save the mute state of all tracks,
 *  and set our mute state to on and all the others to off.
 */
void track::toggleSolo( void )
{
	trackContainer::trackList & tl = m_trackContainer->tracks();

	bool solo_before = FALSE;
	for( trackContainer::trackList::iterator it = tl.begin();
							it != tl.end(); ++it )
	{
		if( *it != this )
		{
			if( ( *it )->m_soloModel.value() )
			{
				solo_before = TRUE;
				break;
			}
		}
	}

	const bool solo = m_soloModel.value();
	for( trackContainer::trackList::iterator it = tl.begin();
							it != tl.end(); ++it )
	{
		if( solo )
		{
			// save mute-state in case no track was solo before
			if( !solo_before )
			{
				( *it )->m_mutedBeforeSolo = ( *it )->isMuted();
			}
			( *it )->setMuted( *it == this ? FALSE : TRUE );
			if( *it != this )
			{
				( *it )->m_soloModel.setValue( FALSE );
			}
		}
		else if( !solo_before )
		{
			( *it )->setMuted( ( *it )->m_mutedBeforeSolo );
		}
	}
}






// ===========================================================================
// trackView
// ===========================================================================

/*! \brief Create a new track View.
 *
 *  The track View is handles the actual display of the track, including
 *  displaying its various widgets and the track segments.
 *
 *  \param _track The track to display.
 *  \param _tcv The track Container View for us to be displayed in.
 *  \todo Is my description of these properties correct?
 */
trackView::trackView( track * _track, trackContainerView * _tcv ) :
	QWidget( _tcv->contentWidget() ),   /*!< The Track Container View's content widget. */
	modelView( NULL, this ),            /*!< The model view of this track */
	m_track( _track ),                  /*!< The track we're displaying */
	m_trackContainerView( _tcv ),       /*!< The track Container View we're displayed in */
	m_trackOperationsWidget( this ),    /*!< Our trackOperationsWidget */
	m_trackSettingsWidget( this ),      /*!< Our trackSettingsWidget */
	m_trackContentWidget( this ),       /*!< Our trackContentWidget */
	m_action( NoAction )                /*!< The action we're currently performing */
{
	setAutoFillBackground( TRUE );
	QPalette pal;
	pal.setColor( backgroundRole(), QColor( 32, 36, 40 ) );
	setPalette( pal );


	m_trackSettingsWidget.setAutoFillBackground( TRUE );
	pal.setColor( m_trackSettingsWidget.backgroundRole(),
							QColor( 56, 60, 72 ) );
	m_trackSettingsWidget.setPalette( pal );

	QHBoxLayout * layout = new QHBoxLayout( this );
	layout->setMargin( 0 );
	layout->setSpacing( 0 );
	layout->addWidget( &m_trackOperationsWidget );
	layout->addWidget( &m_trackSettingsWidget );
	layout->addWidget( &m_trackContentWidget, 1 );

	resizeEvent( NULL );

	setAcceptDrops( TRUE );
	setAttribute( Qt::WA_DeleteOnClose );


	connect( m_track, SIGNAL( destroyed() ), this, SLOT( close() ) );
	connect( m_track,
		SIGNAL( trackContentObjectAdded( trackContentObject * ) ),
			this, SLOT( createTCOView( trackContentObject * ) ),
			Qt::QueuedConnection );

	connect( &m_track->m_mutedModel, SIGNAL( dataChanged() ),
			&m_trackContentWidget, SLOT( update() ) );

	connect( &m_track->m_soloModel, SIGNAL( dataChanged() ),
			m_track, SLOT( toggleSolo() ) );
	// create views for already existing TCOs
	for( track::tcoVector::iterator it =
					m_track->m_trackContentObjects.begin();
			it != m_track->m_trackContentObjects.end(); ++it )
	{
		createTCOView( *it );
	}

	m_trackContainerView->addTrackView( this );
}




/*! \brief Destroy this track View.
 *
 */
trackView::~trackView()
{
}




/*! \brief Resize this track View.
 *
 *  \param _re the Resize Event to handle.
 */
void trackView::resizeEvent( QResizeEvent * _re )
{
	m_trackOperationsWidget.setFixedSize( TRACK_OP_WIDTH, height() - 1 );
	m_trackSettingsWidget.setFixedSize( DEFAULT_SETTINGS_WIDGET_WIDTH,
								height() - 1 );
	m_trackContentWidget.setFixedHeight( height() );
}




/*! \brief Update this track View and all its content objects.
 *
 */
void trackView::update( void )
{
	m_trackContentWidget.update();
	if( !m_trackContainerView->fixedTCOs() )
	{
		m_trackContentWidget.changePosition();
	}
	QWidget::update();
}




/*! \brief Close this track View.
 *
 */
bool trackView::close( void )
{
	m_trackContainerView->removeTrackView( this );
	return( QWidget::close() );
}




/*! \brief Register that the model of this track View has changed.
 *
 */
void trackView::modelChanged( void )
{
	m_track = castModel<track>();
	assert( m_track != NULL );
	connect( m_track, SIGNAL( destroyed() ), this, SLOT( close() ) );
	m_trackOperationsWidget.m_muteBtn->setModel( &m_track->m_mutedModel );
	m_trackOperationsWidget.m_soloBtn->setModel( &m_track->m_soloModel );
	modelView::modelChanged();
}




/*! \brief Undo a change to this track View.
 *
 *  \param _je the Journal Entry to undo.
 */
void trackView::undoStep( journalEntry & _je )
{
	saveJournallingState( FALSE );
	switch( _je.actionID() )
	{
		case MoveTrack:
			if( _je.data().toInt() > 0 )
			{
				m_trackContainerView->moveTrackViewUp( this );
			}
			else
			{
				m_trackContainerView->moveTrackViewDown( this );
			}
			break;
		case ResizeTrack:
			setFixedHeight( tMax<int>( height() +
						_je.data().toInt(),
						MINIMAL_TRACK_HEIGHT ) );
			m_trackContainerView->realignTracks();
			break;
	}
	restoreJournallingState();
}




/*! \brief Redo a change to this track View.
 *
 *  \param _je the Journal Event to redo.
 */
void trackView::redoStep( journalEntry & _je )
{
	journalEntry je( _je.actionID(), -_je.data().toInt() );
	undoStep( je );
}




/*! \brief Start a drag event on this track View.
 *
 *  \param _dee the DragEnterEvent to start.
 */
void trackView::dragEnterEvent( QDragEnterEvent * _dee )
{
	stringPairDrag::processDragEnterEvent( _dee, "track_" +
					QString::number( m_track->type() ) );
}




/*! \brief Accept a drop event on this track View.
 *
 *  We only accept drop events that are of the same type as this track.
 *  If so, we decode the data from the drop event by just feeding it
 *  back into the engine as a state.
 *
 *  \param _de the DropEvent to handle.
 */
void trackView::dropEvent( QDropEvent * _de )
{
	QString type = stringPairDrag::decodeKey( _de );
	QString value = stringPairDrag::decodeValue( _de );
	if( type == ( "track_" + QString::number( m_track->type() ) ) )
	{
		// value contains our XML-data so simply create a
		// multimediaProject which does the rest for us...
		multimediaProject mmp( value, FALSE );
		engine::getMixer()->lock();
		m_track->restoreState( mmp.content().firstChild().toElement() );
		engine::getMixer()->unlock();
		_de->accept();
	}
}




/*! \brief Handle a mouse press event on this track View.
 *
 *  If this track container supports rubber band selection, let the
 *  widget handle that and don't bother with any other handling.
 *
 *  If the left mouse button is pressed, we handle two things.  If
 *  SHIFT is pressed, then we resize vertically.  Otherwise we start
 *  the process of moving this track to a new position.
 *
 *  Otherwise we let the widget handle the mouse event as normal.
 *
 *  \param _me the MouseEvent to handle.
 */
void trackView::mousePressEvent( QMouseEvent * _me )
{
	if( m_trackContainerView->allowRubberband() == TRUE )
	{
		QWidget::mousePressEvent( _me );
	}
	else if( _me->button() == Qt::LeftButton )
	{
		if( engine::getMainWindow()->isShiftPressed() == TRUE )
		{
			m_action = ResizeTrack;
			QCursor::setPos( mapToGlobal( QPoint( _me->x(),
								height() ) ) );
			QCursor c( Qt::SizeVerCursor);
			QApplication::setOverrideCursor( c );
		}
		else
		{
			m_action = MoveTrack;

			QCursor c( Qt::SizeAllCursor );
			QApplication::setOverrideCursor( c );
			// update because in move-mode, all elements in
			// track-op-widgets are hidden as a visual feedback
			m_trackOperationsWidget.update();
		}

		_me->accept();
	}
	else
	{
		QWidget::mousePressEvent( _me );
	}
}




/*! \brief Handle a mouse move event on this track View.
 *
 *  If this track container supports rubber band selection, let the
 *  widget handle that and don't bother with any other handling.
 *
 *  Otherwise if we've started the move process (from mousePressEvent())
 *  then move ourselves into that position, reordering the track list
 *  with moveTrackViewUp() and moveTrackViewDown() to suit.  We make a
 *  note of this in the undo journal in case the user wants to undo this
 *  move.
 *
 *  Likewise if we've started a resize process, handle this too, making
 *  sure that we never go below the minimum track height.
 *
 *  \param _me the MouseEvent to handle.
 */
void trackView::mouseMoveEvent( QMouseEvent * _me )
{
	if( m_trackContainerView->allowRubberband() == TRUE )
	{
		QWidget::mouseMoveEvent( _me );
	}
	else if( m_action == MoveTrack )
	{
		// look which track-widget the mouse-cursor is over
		const trackView * track_at_y =
			m_trackContainerView->trackViewAt(
				mapTo( m_trackContainerView->contentWidget(),
							_me->pos() ).y() );
		// a track-widget not equal to ourself?
		if( track_at_y != NULL && track_at_y != this )
		{
			// then move us up/down there!
			if( _me->y() < 0 )
			{
				m_trackContainerView->moveTrackViewUp( this );
			}
			else
			{
				m_trackContainerView->moveTrackViewDown( this );
			}
			addJournalEntry( journalEntry( MoveTrack, _me->y() ) );
		}
	}
	else if( m_action == ResizeTrack )
	{
		setFixedHeight( tMax<int>( _me->y(), MINIMAL_TRACK_HEIGHT ) );
		m_trackContainerView->realignTracks();
	}
}




/*! \brief Handle a mouse release event on this track View.
 *
 *  \param _me the MouseEvent to handle.
 */
void trackView::mouseReleaseEvent( QMouseEvent * _me )
{
	m_action = NoAction;
	while( QApplication::overrideCursor() != NULL )
	{
		QApplication::restoreOverrideCursor();
	}
	m_trackOperationsWidget.update();

	QWidget::mouseReleaseEvent( _me );
}




/*! \brief Repaint this track View.
 *
 *  \param _pe the PaintEvent to start.
 */
void trackView::paintEvent( QPaintEvent * _pe )
{
	QStyleOption opt;
	opt.initFrom( this );
	QPainter p( this );
	style()->drawPrimitive( QStyle::PE_Widget, &opt, &p, this );
}




/*! \brief Create a trackContentObject View in this track View.
 *
 *  \param _tco the trackContentObject to create the view for.
 *  \todo is this a good description for what this method does?
 */
void trackView::createTCOView( trackContentObject * _tco )
{
	_tco->createView( this );
}





#include "track.moc"


#endif
