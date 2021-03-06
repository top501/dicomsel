// PixelRepresentation.h
#ifndef DICOM_PIXELREPRESENTATION_TAG_H
#define DICOM_PIXELREPRESENTATION_TAG_H

#include "Virtuals/Type.h"
#include "DicomTag.h"

namespace dicom
{

namespace tag
{

class CPixelRepresentation : public CDicomTag
{
private :
    bool uiUnsignedFlag;
public :
	/**
	 * Constructor.
	 * Call the root constructor.
	 *********************
	 * @param _i32Group [const int32]	: tag's group.
	 * @param _i32Element [const int32] : tag's element.
	 * @param _i32Length [const int32]	: tag's length.
	 *********************
	 * @pre Group == 0x0028 && Element 0x0103
	 *********************
	 * @see dicom::tag::CDicomTag
	 */
    explicit				CPixelRepresentation					( const int32 _i32Group, const int32 _i32Element, const int32 _i32Length );

    const bool				isUnsignedFlag							( void ) const ;

	/**
	 * Read value and update position in input file.	 
	 *********************
	 * @param _File [dicom::io::CDicomFile &] : Full name of the parsed file	 
	 *********************
	 * @pre file's offset + tag's length < file's length.
	 * @pre file open.
	 * @post file's offset <= file's length.	 
	 ****************************************
	 * @exception std::exception for basic_istream::read	 
	 ****************************************
	 * @see dicom::tag::CDicomTag::ReadMeIntoFile	 
	 ****************************************
	 * @return const uint32 : final offset after read
	 */
	virtual const uint32	SetValue								( dicom::io::CDicomFile & _File ) ;

	/**
	 * DP VISITOR.
	 * According to the visitor (image, list, etc...) the Time used or not.
	 * If this tag used by the visitor, the SetValue() function called.
	 *********************
	 * @param _pVisitor [dicom::visitor::CAbstractDicomFile * const] : Vistor
	 *********************
	 * @pre visitor not null
	 *********************
	 * @return const uint32 : final offset after read
	 */
    const uint32			Visit									( dicom::visitor::CAbstractDicomFile * const _pVisitor ) ;
};

}// end namespace tag

}// end namespace

#endif // DICOM_PIXELREPRESENTATION_TAG_H
