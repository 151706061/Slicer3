
#########################################################
#
if {0} { ;# comment

  GridSWidget  - manages slice plane interactions

# TODO : 

}
#
#########################################################
# ------------------------------------------------------------------
#                             GridSWidget
# ------------------------------------------------------------------
#
# The class definition - define if needed (not when re-sourcing)
#
if { [itcl::find class GridSWidget] == "" } {

  itcl::class GridSWidget {

    inherit SWidget

    constructor {args} {}
    destructor {}

    public variable layer "label"  ;# which slice layer to show the grid for
    public variable rgba ".5 .5 1 .3"  ;# grid color
    public variable cutoff "5"  ;# don't show grid if it's less than 'cutoff' screen pixels

    # methods
    method processEvent { {caller ""} } {}
    method updateGrid { } {}
    method resetGrid { } {}
    method addGridLine { startPoint endPoint } {}
  }
}

# ------------------------------------------------------------------
#                        CONSTRUCTOR/DESTRUCTOR
# ------------------------------------------------------------------
itcl::body GridSWidget::constructor {sliceGUI} {

  $this configure -sliceGUI $sliceGUI
 
  # create grid display parts
  set o(gridPolyData) [vtkNew vtkPolyData]
  set o(gridLines) [vtkNew vtkCellArray]
  $o(gridPolyData) SetLines $o(gridLines)
  set o(gridPoints) [vtkNew vtkPoints]
  $o(gridPolyData) SetPoints $o(gridPoints)

  set o(gridMapper) [vtkNew vtkPolyDataMapper2D]
  set o(gridActor) [vtkNew vtkActor2D]
  $o(gridMapper) SetInput $o(gridPolyData)
  $o(gridActor) SetMapper $o(gridMapper)
  eval [$o(gridActor) GetProperty] SetColor [lrange $rgba 0 2]
  eval [$o(gridActor) GetProperty] SetOpacity [lindex $rgba 3]
  [$_renderWidget GetRenderer] AddActor2D $o(gridActor)
  lappend _actors $o(gridActor)

  #
  # set up observers on sliceGUI and on sliceNode
  # - track them so they can be removed in the destructor
  #
  set _guiObserverTags ""

  lappend _guiObserverTags [$sliceGUI AddObserver DeleteEvent "itcl::delete object $this"]

  set events {  "MouseMoveEvent" "UserEvent" }
  foreach event $events {
    lappend _guiObserverTags [$sliceGUI AddObserver $event "$this processEvent $sliceGUI"]    
  }

  set node [[$sliceGUI GetLogic] GetSliceNode]
  lappend _nodeObserverTags [$node AddObserver DeleteEvent "itcl::delete object $this"]
  lappend _nodeObserverTags [$node AddObserver AnyEvent "$this processEvent $node"]
}


itcl::body GridSWidget::destructor {} {

  if { [info command $sliceGUI] != "" } {
    foreach tag $_guiObserverTags {
      $sliceGUI RemoveObserver $tag
    }
  }

  if { [info command $_sliceNode] != "" } {
    foreach tag $_nodeObserverTags {
      $_sliceNode RemoveObserver $tag
    }
  }

  if { [info command $_renderer] != "" } {
    foreach a $_actors {
      $_renderer RemoveActor2D $a
    }
  }
}



# ------------------------------------------------------------------
#                             METHODS
# ------------------------------------------------------------------

#
# handle interactor events
#
itcl::body GridSWidget::processEvent { {caller ""} } {

  if { [info command $sliceGUI] == "" } {
    # the sliceGUI was deleted behind our back, so we need to 
    # self destruct
    itcl::delete object $this
    return
  }

  foreach {x y} [$_interactor GetEventPosition] {}
  $this queryLayers $x $y
  set xyToRAS [$_sliceNode GetXYToRAS]
  set ras [$xyToRAS MultiplyPoint $x $y 0 1]
  foreach {r a s t} $ras {}

  set node [[$sliceGUI GetLogic] GetSliceNode]
  if { $caller == $node } {
    $this updateGrid
    return
  }

  set event [$sliceGUI GetCurrentGUIEvent] 
  if { $caller == $sliceGUI } {

    switch $event {

      "MouseMoveEvent" {
        #
        # highlight the current grid cell
        #

        # update the actors...
      }
    }

    return
  }

}

itcl::body GridSWidget::resetGrid { } {

  set idArray [$o(gridLines) GetData]
  $idArray Reset
  $idArray InsertNextTuple1 0
  $o(gridPoints) Reset
  $o(gridLines) SetNumberOfCells 0

}

itcl::body GridSWidget::addGridLine { startPoint endPoint } {

  set startPoint [lrange $startPoint 0 2]
  set endPoint [lrange $endPoint 0 2]
  set startIndex [eval $o(gridPoints) InsertNextPoint $startPoint]
  set endIndex [eval $o(gridPoints) InsertNextPoint $endPoint]

  set cellCount [$o(gridLines) GetNumberOfCells]
  set idArray [$o(gridLines) GetData]
  $idArray InsertNextTuple1 2
  $idArray InsertNextTuple1 $startIndex
  $idArray InsertNextTuple1 $endIndex
  $o(gridLines) SetNumberOfCells [expr $cellCount + 1]

}


#
# make the grid object
#
itcl::body GridSWidget::updateGrid { } {

  $this resetGrid

  set layer "background"

  #
  # check the size cutoff
  # - map a single pixel from IJK to XY and check the size
  #
  set ijkToXY [vtkMatrix4x4 New]
  $ijkToXY DeepCopy [[$_layers($layer,logic) GetXYToIJKTransform] GetMatrix]
  $ijkToXY SetElement 0 3  0
  $ijkToXY SetElement 1 3  0
  $ijkToXY SetElement 2 3  0
  $ijkToXY Invert
  foreach {x y z w} [$ijkToXY MultiplyPoint 1 1 1 0] {}
  if { $x < $cutoff && $y < $cutoff } {
    $o(gridActor) SetVisibility 0
    return
  } else {
    $o(gridActor) SetVisibility 1
  }


  #
  # determine which slice plane to display 
  # - since this is an orthogonal projection, all slices will look the same,
  #   so once we know which to draw, we only need to draw a single one
  # - choose the two indices that have the largest change with respect to XY
  # - make two sets of lines, one along the rows and one along the columns
  #
  set xyToIJK [vtkMatrix4x4 New]
  $xyToIJK DeepCopy [[$_layers($layer,logic) GetXYToIJKTransform] GetMatrix]
  $xyToIJK SetElement 0 3  0
  $xyToIJK SetElement 1 3  0
  $xyToIJK SetElement 2 3  0
  foreach {i j k l} [$xyToIJK MultiplyPoint 1 1 0 0] {}
  foreach v {i j k l} { set $v [expr abs([set $v])] }

  if { $i < $j && $i < $k } { set rowAxis 1; set colAxis 2 }
  if { $j < $i && $j < $k } { set rowAxis 0; set colAxis 2 }
  if { $k < $i && $k < $j } { set rowAxis 0; set colAxis 1 }

  set dims [$_layers($layer,image) GetDimensions]
  set rowDims [lindex $dims $rowAxis]
  set colDims [lindex $dims $colAxis]

  $ijkToXY DeepCopy [[$_layers($layer,logic) GetXYToIJKTransform] GetMatrix]
  $ijkToXY Invert

  set startPoint "0 0 0 1"
  set endPoint "0 0 0 1"
  set endPoint [lreplace $endPoint $colAxis $colAxis $colDims]

  for {set row 0} {$row <= $rowDims} {incr row} {
    set startPoint [lreplace $startPoint $rowAxis $rowAxis $row]
    set endPoint [lreplace $endPoint $rowAxis $rowAxis $row]
    set xyStartPoint [eval $ijkToXY MultiplyPoint $startPoint]
    set xyEndPoint [eval $ijkToXY MultiplyPoint $endPoint]
    $this addGridLine $xyStartPoint $xyEndPoint
  }

  set startPoint "0 0 0 1"
  set endPoint "0 0 0 1"
  set endPoint [lreplace $endPoint $rowAxis $rowAxis $rowDims]
  for {set col 0} {$col <= $colDims} {incr col} {
    set startPoint [lreplace $startPoint $colAxis $colAxis $col]
    set endPoint [lreplace $endPoint $colAxis $colAxis $col]
    set xyStartPoint [eval $ijkToXY MultiplyPoint $startPoint]
    set xyEndPoint [eval $ijkToXY MultiplyPoint $endPoint]
    $this addGridLine $xyStartPoint $xyEndPoint
  }

  $ijkToXY Delete
  $xyToIJK Delete

  [$sliceGUI GetSliceViewer] RequestRender
}

proc GridSWidget::AddGrid {} {
  foreach sw [itcl::find objects -class SliceSWidget] {
    set sliceGUI [$sw cget -sliceGUI]
    if { [info command $sliceGUI] != "" } {
      GridSWidget #auto [$sw cget -sliceGUI]
    }
  }
}

proc GridSWidget::RemoveGrid {} {
  foreach pw [itcl::find objects -class GridSWidget] {
    itcl::delete object $pw
  }
}

proc GridSWidget::ToggleGrid {} {
  if { [itcl::find objects -class GridSWidget] == "" } {
    GridSWidget::AddGrid
  } else {
    GridSWidget::RemoveGrid
  }
}
