/* Source file for the R3 scene node class */



////////////////////////////////////////////////////////////////////////
// INCLUDE FILES
////////////////////////////////////////////////////////////////////////

#include "R3Graphics.h"





////////////////////////////////////////////////////////////////////////
// PKG INITIALIZATION FUNCTIONS
////////////////////////////////////////////////////////////////////////

int 
R3InitSceneNode()
{
  /* Return success */
  return TRUE;
}



void 
R3StopSceneNode()
{
}



////////////////////////////////////////////////////////////////////////
// MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////

R3SceneNode::
R3SceneNode(R3Scene *scene) 
  : scene(NULL),
    scene_index(-1),
    parent(NULL),
    parent_index(-1),
    children(),
    elements(),
    transformation(R3identity_affine),
    name(NULL),
    bbox(FLT_MAX, FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX)
{
  // Insert node into scene
  if (scene) {
    scene->InsertNode(this);
  }
}



R3SceneNode::
~R3SceneNode(void)
{
  // Remove elements
  for (int i = 0; i < elements.NEntries(); i++) {
    R3SceneElement *element = elements.Kth(i);
    RemoveElement(element);
  }

  // Remove children
  for (int i = 0; i < children.NEntries(); i++) {
    R3SceneNode *child = children.Kth(i);
    RemoveChild(child);
  }

  // Remove node from parent
  if (parent) {
    parent->RemoveChild(this);
  }

  // Remove node from scene
  if (scene) {
    scene->RemoveNode(this);
  }

  // Delete name
  if (name) free(name);
}



void R3SceneNode::
InsertChild(R3SceneNode *node) 
{
  // Check scene
  assert(node->scene == scene);
  assert(node->scene_index >= 0);

  // Insert node into children
  assert(!node->parent);
  assert(node->parent_index == -1);
  node->parent = this;
  node->parent_index = children.NEntries();
  children.Insert(node);

  // Invalidate bounding box
  InvalidateBBox();
}



void R3SceneNode::
RemoveChild(R3SceneNode *node) 
{
  // Remove node from children
  assert(node->parent == this);
  RNArrayEntry *entry = children.KthEntry(node->parent_index);
  assert(children.EntryContents(entry) == node);
  R3SceneNode *tail = children.Tail();
  children.EntryContents(entry) = tail;
  tail->parent_index = node->parent_index;
  children.RemoveTail();
  node->parent = NULL;
  node->parent_index = -1;

  // Invalidate bounding box
  InvalidateBBox();
}



void R3SceneNode::
InsertElement(R3SceneElement *element) 
{
  // Update goemetry
  assert(!element->node);
  element->node = this;

  // Insert element
  elements.Insert(element);

  // Invalidate bounding box
  InvalidateBBox();
}



void R3SceneNode::
RemoveElement(R3SceneElement *element) 
{
  // Update goemetry
  assert(element->node == this);
  element->node = NULL;

  // Remove element
  elements.Remove(element);

  // Invalidate bounding box
  InvalidateBBox();
}



void R3SceneNode::
SetTransformation(const R3Affine& transformation)
{
  // Set transformation
  this->transformation = transformation;

  // Invalidate bounding box
  InvalidateBBox();
}



void R3SceneNode::
SetName(const char *name)
{
  // Set name
  if (this->name) free(this->name);
  if (name) this->name = strdup(name);
  else this->name = NULL;
}



RNBoolean R3SceneNode::
Intersects(const R3Ray& ray, 
  R3SceneNode **hit_node, R3SceneElement **hit_element, R3Shape **hit_shape,
  R3Point *hit_point, R3Vector *hit_normal, RNScalar *hit_t) const
{
  // Check if ray intersects bounding box
  if (!R3Intersects(ray, BBox())) return FALSE;

  // Temporary variables
  R3SceneNode *closest_node = NULL;
  RNScalar closest_t = FLT_MAX;
  R3SceneNode *node;
  R3SceneElement *element;
  R3Shape *shape;
  R3Point point;
  R3Vector normal;
  RNScalar t;

  // Apply inverse transformation to ray
  R3Ray node_ray = ray;
  node_ray.InverseTransform(transformation);

  // Find closest element intersection
  for (int i = 0; i < elements.NEntries(); i++) {
    R3SceneElement *element = elements.Kth(i);
    if (element->Intersects(node_ray, &shape, &point, &normal, &t)) {
      if (t < closest_t) {
        if (hit_node) *hit_node = (R3SceneNode *) this;
        if (hit_element) *hit_element = element;
        if (hit_shape) *hit_shape = shape; 
        if (hit_point) *hit_point = point; 
        if (hit_normal) *hit_normal = normal; 
        closest_node = (R3SceneNode *) this;
        closest_t = t;
      }
    }
  }

  // Find closest node intersection
  for (int i = 0; i < children.NEntries(); i++) {
    R3SceneNode *child = children.Kth(i);
    if (child->Intersects(node_ray, &node, &element, &shape, &point, &normal, &t)) {
      if (t < closest_t) {
        if (hit_node) *hit_node = node;
        if (hit_element) *hit_element = element;
        if (hit_shape) *hit_shape = shape; 
        if (hit_point) *hit_point = point; 
        if (hit_normal) *hit_normal = normal; 
        closest_node = node;
        closest_t = t;
      }
    }
  }

  // Check if found hit
  if (!closest_node) return FALSE;

  // Transform result back into parent's coordinate system
  RNScalar scale_factor = transformation.ScaleFactor();
  if (hit_point) hit_point->Transform(transformation); 
  if (hit_normal) hit_normal->Transform(transformation); 
  if (hit_t) *hit_t = closest_t * scale_factor;

  // Return success
  return TRUE;
}



void R3SceneNode::
Draw(const R3DrawFlags draw_flags) const
{
  // Push transformation
  transformation.Push();

  // Draw elements
  for (int i = 0; i < elements.NEntries(); i++) {
    R3SceneElement *element = elements.Kth(i);
    element->Draw(draw_flags);
  }

  // Draw children
  for (int i = 0; i < children.NEntries(); i++) {
    R3SceneNode *child = children.Kth(i);
    child->Draw(draw_flags);
    child->BBox().Outline();
  }

  // Pop transformation
  transformation.Pop();
}



void R3SceneNode::
UpdateBBox(void)
{
  // Initialize bounding box
  bbox = R3null_box;

  // Add bounding box of elements
  for (int i = 0; i < elements.NEntries(); i++) {
    R3SceneElement *element = elements.Kth(i);
    R3Box element_bbox = element->BBox();
    element_bbox.Transform(transformation);
    bbox.Union(element_bbox);
  }
    
  // Add bounding box of children nodes
  for (int i = 0; i < children.NEntries(); i++) {
    R3SceneNode *child = children.Kth(i);
    R3Box child_bbox = child->BBox();
    child_bbox.Transform(transformation);
    bbox.Union(child_bbox);
  }
}



void R3SceneNode::
InvalidateBBox(void)
{
  // Invalidate bounding box
  bbox[0][0] = FLT_MAX;

  // Invalidate parent's bounding box
  if (parent) parent->InvalidateBBox();
}



