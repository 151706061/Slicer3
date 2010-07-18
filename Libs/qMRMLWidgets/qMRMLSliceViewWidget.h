#ifndef __qMRMLSliceViewWidget_h
#define __qMRMLSliceViewWidget_h

// Qt includes
#include <QWidget>

// CTK includes
#include <ctkPimpl.h>

// qMRMLWidget includes
#include "qMRMLWidget.h"

#include "qMRMLWidgetsExport.h"

class qMRMLSliceViewWidgetPrivate;
class qMRMLSliceControllerWidget;
class vtkMRMLScene;
class vtkMRMLNode;
class vtkMRMLSliceNode;
class vtkMRMLSliceCompositeNode;
class vtkImageData;
class ctkVTKSliceView;

class vtkMRMLSliceLogic;

class QMRML_WIDGETS_EXPORT qMRMLSliceViewWidget : public qMRMLWidget
{
  Q_OBJECT
  Q_PROPERTY(QString sliceOrientation READ sliceOrientation WRITE setSliceOrientation)
public:
  /// Superclass typedef
  typedef qMRMLWidget Superclass;
  
  /// Constructors
  explicit qMRMLSliceViewWidget(QWidget* parent = 0);
  virtual ~qMRMLSliceViewWidget(){}

  /// Get slice controller
  qMRMLSliceControllerWidget* sliceController()const;

  /// \sa qMRMLSliceControllerWidget::mrmlSliceNode()
  /// \sa setMRMLSliceNode()
  vtkMRMLSliceNode* mrmlSliceNode()const;

  /// \sa qMRMLSliceControllerWidget::sliceOrientation()
  /// \sa setSliceOrientation()
  QString sliceOrientation()const;

  /// \sa qMRMLSliceControllerWidget::imageData()
  /// \sa setImageData();
  vtkImageData* imageData()const;

  /// \sa qMRMLSliceControllerWidget::mrmlSliceCompositeNode()
  vtkMRMLSliceCompositeNode* mrmlSliceCompositeNode()const;

  /// \sa qMRMLSliceControllerWidget::sliceViewName()
  /// \sa setSliceViewName()
  QString sliceViewName()const;

  /// \sa qMRMLSliceControllerWidget::sliceViewName()
  /// \sa sliceViewName()
  void setSliceViewName(const QString& newSliceViewName);

public slots:

  void setMRMLScene(vtkMRMLScene * newScene);

  /// \sa qMRMLSliceControllerWidget::setMRMLSliceNode()
  /// \sa mrmlSliceNode()
  void setMRMLSliceNode(vtkMRMLSliceNode* newSliceNode);

  /// \sa qMRMLSliceControllerWidget::setImageData()
  /// \sa imageData()
  void setImageData(vtkImageData* newImageData);
  
  /// \sa qMRMLSliceControllerWidget::setSliceOrientation()
  /// \sa sliceOrientation()
  void setSliceOrientation(const QString& orienation);

  /// Fit slices to background
  void fitSliceToBackground();

protected:

  /// Get a reference to the underlying Slice View
  ctkVTKSliceView * sliceView()const;

private:
  CTK_DECLARE_PRIVATE(qMRMLSliceViewWidget);
};

#endif
