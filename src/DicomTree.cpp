/*
 * ---------------------------------------------------------------------------
 *
 * DicomSel: DICOM Image Selector
 *
 * Copyright (C) 2005, 2006 IRCAD
 * Authors: Benjamin Gaillard, Marc-Aurèle Mörk, Guillaume Spitz
 *
 * ---------------------------------------------------------------------------
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * ---------------------------------------------------------------------------
 */


// wxWidgets
#include <wx/wxprec.h>
#ifdef __BORLANDC__
# pragma hdrstop
#endif
//#ifndef WX_PRECOMP
# include <wx/string.h>
# include <wx/filename.h>
# include <wx/dir.h>
# include <wx/gdicmn.h>
# include <wx/window.h>
# include <wx/utils.h>
# include <wx/validate.h>
# include <wx/imaglist.h>
# include <wx/treectrl.h>
# include <wx/progdlg.h>
# include <wx/msgdlg.h>
# include <wx/image.h>
# include <wx/bitmap.h>
//#endif

// STL
#include <vector>
#include <map>
#include <algorithm>

// Assertions
#include <cassert>

// Current module
#include <dicomsel/DicomCollection.h>
#include <dicomsel/DicomFile.h>
#include <dicomsel/DicomTree.h>

class wxBitmap;


namespace dicomsel
{

#include "images/dir.xpm"
#include "images/patient.xpm"
#include "images/study.xpm"
#include "images/series.xpm"
#include "images/image.xpm"


enum { PROGRESS_TOTAL = 1 << (sizeof( int ) * 4 - 2) };

BEGIN_EVENT_TABLE( DicomTree, wxTreeCtrl )
    EVT_TREE_SEL_CHANGED( -1, DicomTree::OnSelectionChanged )
END_EVENT_TABLE()


class DicomTree::Files
{
public:

    void BuildTree( wxTreeCtrl* const tree, const ::wxTreeItemId& parent,
		    const int icon = 1 )
    {
	int i = 0;

	std::sort( m_files.begin(), m_files.end() );
	for( std::vector< wxString >::const_iterator element
		 = m_files.begin();
	     element != m_files.end();
	     ++element )
	{
	    DicomFile* const file = new DicomFile( *element );
	    file->SetId( tree->AppendItem( parent,
					   wxString::Format( wxT( "%d" ),
							     ++i ),
					   icon, -1, file ) );
	}
    }

    void Append( const wxString& filename )
    {
	m_files.push_back( filename );
    }

private:
    std::vector< wxString > m_files;
};

template< class Type >
class DicomTree::TreeMap
{
public:
    ~TreeMap( void )
    {
	for( std::map< const wxString, void* >::const_iterator element =
		 m_map.begin();
	     element != m_map.end();
	     ++element )
	{
	    Type* const object
		= reinterpret_cast< Type* const >( element->second );
	    if ( object != NULL ) delete object;
	}
    }

    Type& operator []( const wxString& name )
    {
	void*& object = m_map[name];
	if( object == NULL ) object = new Type;
	return *(reinterpret_cast< Type* >( object ));
    }

    int getSize( void ) const
    {
	return m_map.size();
    }

    void BuildTree( wxTreeCtrl* const tree, const wxTreeItemId& parent,
		    const int icon = 1 ) const
    {
	for( std::map< const wxString, void* >::const_iterator element =
		 m_map.begin();
	     element != m_map.end();
	     ++element )
	{
	    Type* const object = reinterpret_cast< Type* >( element->second );
	    if ( object != NULL )
	    {
		wxTreeItemId item =
		    tree->AppendItem( parent, element->first, icon );
		object->BuildTree( tree, item, icon + 1 );
	    }
	}
    }

private:
    // FIXME: we use void* instead of Type* since it seems to confuse GCC
    std::map< const wxString, void* > m_map;
};


DicomTree::DicomTree( wxWindow* parent, wxWindowID id, const wxPoint& pos,
		      const wxSize& size, long style,
		      const wxValidator& validator, const wxString& name )
:   wxTreeCtrl      ( parent, id, pos, size, style, validator, name ),
    m_files         ( NULL ),
    m_currentFile   ( NULL ),
    m_total         ( 0 ),
    m_progress      ( 0 ),
    m_progressDialog( NULL )
{
    // Create icon list
    wxImageList* const imageList = new wxImageList( 16, 16, TRUE, 5 );

    // Add icons to the list
    imageList->Add( wxBitmap( dir_xpm     ) );
    imageList->Add( wxBitmap( patient_xpm ) );
    imageList->Add( wxBitmap( study_xpm   ) );
    imageList->Add( wxBitmap( series_xpm  ) );
    imageList->Add( wxBitmap( image_xpm   ) );

    // Set the used icons for the tree
    AssignImageList( imageList );
}

DicomTree::~DicomTree( void )
{
    // Destroy progress dialog and file list if they still exist (but they
    // shouldn't)
    if( m_progressDialog != NULL ) m_progressDialog->Destroy();
    if( m_files != NULL ) delete m_files;
}

bool DicomTree::OpenDirectory( const wxString& name )
{
    bool ret = false;

    // Require directory existence
    if( wxDir::Exists( name ) )
    {
	wxBeginBusyCursor();
	m_files = new TreeMap< TreeMap< TreeMap< Files > > >;

	{
	    DicomCollection collection( this );
	    collection.ScanDirectory( name );
	}

	if( m_files != NULL)
	{
	    if( m_files->getSize() != 0 )
	    {
		wxTreeItemId root = GetRootItem();

		if( root.IsOk() ) Delete( root );
		root = AddRoot( name, 0 );
		m_files->BuildTree( this, root );
		Expand( root );

		m_currentFile = NULL;
		ret = true;
	    }
	    else
	    {
		wxMessageBox( wxT( "No DICOM file found in the selected "
				   "directory." ), wxT( "No file found" ),
			      wxOK | wxICON_ERROR, this );
	    }

	    delete m_files;
	    m_files = NULL;
	}

	wxEndBusyCursor();
    }
    else
    {
	wxMessageBox( wxT( "The selected directory doesn't exist." ),
		      wxT( "Nonexistant directory" ),
		      wxOK | wxICON_ERROR, this );
    }

    return ret;
}

void DicomTree::CloseDirectory( void )
{
    const wxTreeItemId root = GetRootItem();
    if( root.IsOk() ) Delete( root );
    m_currentFile = NULL;
}

wxImage DicomTree::GetImage( void )
{
    return m_currentFile != NULL ? m_currentFile->GetImage() : wxNullImage;
}

wxBitmap DicomTree::GetBitmap( void )
{
    return m_currentFile != NULL ? m_currentFile->GetImage() : wxNullBitmap;
}

void DicomTree::ParsingDirectory( void )
{
    // Create progress dialog
    m_progressDialog = new wxProgressDialog(
	wxT( "Progress" ),
	wxT( "Scanning directory..." ),
	PROGRESS_TOTAL, NULL,
	wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_CAN_ABORT |
	    wxPD_ELAPSED_TIME | wxPD_ESTIMATED_TIME | wxPD_REMAINING_TIME
    );

    // Force window drawing
    m_progressDialog->Refresh();
    m_progressDialog->Update( 0 );
}

bool DicomTree::BeginParsing( int total )
{
    // Initialize variables
    m_total    = total != 0 ? total : 1; // Avoid division by zero exception
    m_progress = 0;

    // Update progress dialog message
    return m_progressDialog->Update( 0, wxT( "Parsing DICOM files..." ) );
}

void DicomTree::AddFile( const wxString& name, const wxString& study,
			 const wxString& series, const wxString& file )
{
    // Add this file to the list
    (*m_files)[name][study][series].Append( file );
}

bool DicomTree::ProgressParsing( void )
{
    // Display incremented progress counter
    if( m_progressDialog != NULL )
    {
	return m_progressDialog->Update( ++m_progress * PROGRESS_TOTAL
					 / m_total );
    }

    // If dialog is unbeknownstly destroyed, continue anyway
    return true;
}

void DicomTree::AbortParsing( void )
{
    // Destroy file list
    if( m_files != NULL )
    {
	delete m_files;
	m_files = NULL;
    }
}

void DicomTree::EndParsing( void )
{
    // Close progress dialog
    m_progressDialog->Destroy();

    // Reset variables
    m_progress       = 0;
    m_progressDialog = NULL;
}

int DicomTree::OnCompareItems( const wxTreeItemId& item1,
			       const wxTreeItemId& item2 )
{
    assert( item1.IsOk() && item2.IsOk() );

    // Directories first
    if( ItemHasChildren( item1 ) != ItemHasChildren( item2 ) )
    {
	return ItemHasChildren( item1 ) ? -1 : 1;
    }

    // Compare alphanumerically
    if( GetItemText( item1 ) < GetItemText( item2 ) ) return -1;
    if( GetItemText( item1 ) > GetItemText( item2 ) ) return  1;
    return 0;
}

void DicomTree::OnSelectionChanged( wxTreeEvent& event )
{
    if( m_currentFile != NULL ) m_currentFile->Free();

    const wxTreeItemId item = event.GetItem();
    m_currentFile = item.IsOk() ?
	reinterpret_cast< DicomFile* >( GetItemData( item ) ) : NULL;

    event.Skip();
}

} // namespace dicomsel

/* End of File */
