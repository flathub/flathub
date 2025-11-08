# Running an Example OpenFOAM Tutorial Case

You can test your OpenFOAM Flatpak installation using **lid-driven cavity flow** tutorial.
For more details, refer to the official documentation:
[OpenFOAM Tutorial Guide — 2.1 Lid-driven cavity flow](https://www.openfoam.com/documentation/tutorial-guide/2-incompressible-flow/2.1-lid-driven-cavity-flow)

---

### Run the Example

```bash
flatpak run com.openfoam.OpenFOAM
echo "$FOAM_RUN" # display working directory for OpenFOAM case
echo "$FOAM_TUTORIALS" # display tutorial directory
mkdir -p "$FOAM_RUN" # create working directory
cd "$FOAM_RUN"
cp -r "$FOAM_TUTORIALS/incompressible/icoFoam/cavity/cavity" ~/Desktop # copy example files to user desktop folder
cd ~/Desktop/cavity
blockMesh
icoFoam
paraFoam -touch-all
ls -l # list the files generated
exit # exit the OpenFOAM environment
```

---

### View the Results in ParaView

ParaView can be used to visualize the results.
It is also available as a Flatpak package.

```bash
flatpak install org.paraview.ParaView # install ParaView
flatpak run org.paraview.ParaView ~/Desktop/cavity/cavity.foam # open OpenFOAM case
```

