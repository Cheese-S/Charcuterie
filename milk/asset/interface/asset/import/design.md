# High level
- SceneImporter -> IR (UFBX, tinygltf etc).
- IR -> list of build targets.
  - each build target gets assigned some GUID. 
  - register references in asset DB
- BuildManager -> build(some target)
  - insert some sort of cache here to avoid redundant build
  - query asset DB, find dependencies. recursively build dependencies. 
  - each asset builder achtually can spawn small size jobs

# for v1
- just build the IR and then dumbly build assets + references
