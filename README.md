<img src=".github/gerda-logo.png" align="left"  height="80"/>

# gerda-factory

JSON-configurable set of tools to generate statistical realizations of typical
GERDA energy spectra. The `gerda-factory` program allows to take into account
(randomly) generic energy-dependent deformations, and therefore can be useful
for studies of systematic uncertainties.

### `gerda-fake-gen`

1. compile the project by running `make` at the top of the directory tree. The
   only external dependency is [ROOT](https://root.cern.ch/).
2. acquire the official GERDA PDFs:
   ```console
   cd data
   ./get-pdfs
   ```
3. write a JSON config file (examples under `config/`, see last section for a
   brief explanation of the syntax)
4. run `gerda-fake-gen <json-file>` to generate a random experiment

### `gerda-factory`

1. compile the project by running `make` at the top of the directory tree.
2. acquire the official GERDA PDFs:
   ```console
   cd data
   ./get-pdfs
   ```
3. compute distortion functions from the different PDF versions:
   ```console
   cd data
   ./compute-distortions
   ```
3. write a JSON config file (examples under `config/*-systematics.json`, see
   next section for a brief explanation of the syntax)
4. run `gerda-factory <json-file>` to generate a set of random experiments with
   random distortion functions applied

## Config files

The JSON config file for the `gerda-fake-gen` program begins with some general settings:
```js
{
    "id" : "phIIAfterLAr",  // model name
    "logging" : "info",     // verbosity level, choose between {debug, detail, info, warning, error}
    "output" : {  // output settings
        "file" : "../results/phIIAfterLAr-exp-pool.root:object_name",  // output filename (and ROOT object name)
        "number-of-bins" : 8000,
        "xaxis-range" : [0, 8000]
    },
    "range-for-counts" : [565, 2000],  // histogram range in which the number of counts specified in the following
                                       // should be considered
    "gerda-pdfs" : "../data/gerda-pdfs/gerda-pdfs-latest",  // default value for the location of the GERDA PDFs
    "hist-name" : "M1_enrBEGe",  // default name of the histogram to be searched for in the ROOT files
```
then a large section follows to configure the generation model, where
everything about each component can be specified in a modular fashion:
```js
    "components" : [  // here you must specify a list of PDFs you want to use
        { ... }, { ... }, ...
    ]
```
The `"components"` array specifies a list of components (PDFs). The array is
filled with JSON objects that can be of multiple types. This special syntax is
the same employed in the [gerda-fitter](https://github.com/gipert/gerda-fitter)
project.

As instance, one might want to use the GERDA PDFs distributed within
[gerda-mage-sim](https://github.com/mppmu/gerda-mage-sim) using the following
structure:
```js
{  // this is an object in the "components" array presented above
    "gerda-pdfs" : "../data/gerda-pdfs/v1.0"  // the gerda-pdfs path might be set here to override the global one
    "part": "cables/cables_all",
    "components" : { // list of components from the 'cables/cables_all' part
        "Th228-cables" : {  // the key isn't very important, but please choose a unique name!
            "isotope" : { "Tl208-lar" : 0.3539, "Bi212-lar" : 1 },  // specify a mixture of isotopes
            "amount-cts" : 666  // say here how many counts do you want to sample from the PDF
                                // (NB: remember the "range-for-counts" parameter above
        },
        "Co60-cables" : {
            "isotope": "Co60-lar", // no mixture here
            ...
        },
        ...
    }
},
{  // this is an object in the "components" array presented above
    "part": {  // you can also specify a mixture of parts!
        "cables/cables_all" : 0.2,
        "cables/hv_cables"  : 0.8,
        ...
    },
    "components" : { ... }
}
```
or even provide manually a ROOT histogram:
```js
{
    "root-file" : "../data/gerda-pdfs/gerda-pdfs-latest/alphas/analytic/pdf-functions.root",
    "components" : {
        "alpha-offset" : {
            "hist-name" : "flat",
            "amount-cts" : 23
        },
        ...
    }
},
```

### Additional configs for `gerda-factory`

The `gerda-factory` program needs distorting functions as input, therefore an
additional JSON block `"pdf-distortions"` must be present in the config file.
The syntax is the following:
```js
    "pdf-distortions" : {
        "prefix" : "../data/distortions",  // global prefix where the files/folders will be searched for
        "global" : {  // category of distortions that should be applied on all the components
                      // the structure of the folders must be organized in the same way as the GERDA PDFs
                      // releases and the name of the histogram must match
            "distortion-type1" : {  // set the deformation type (not important)
                "pdfs" : [
                    "folder1",  // list here the folder names
                    ...
                ],
                "interpolate": true  // can randomly reduce the magnitude of the distortion
                                     // by mixing it with the unitary distortion. must be used
                                     // with care, please see note below
            },
            ...
        },
        "specific" : {  // category of distortions that have to be applied to single components
            "component1"  : { "pdfs" : [ "file1.root" ] }, // the name of the component must be listed also in the
                                                           // list of model components above!
            "component2" : {
                "pdfs" : [
                    "file2.root:objname",  // single ROOT file with eventual object name
                    "file3.root",
                    ...
                ],
                "interpolate" : true,
            }
        }
    }
```

**Note:** interpolation is performed with respect to the unitary distortion. In
practice, after randomly selecting a distortion from a certain group, an
additional random number `w` is drawn from a uniform distribution in [0,1].
This number is used to weight the distortion `D` together with the unitary
distortion `U` according to the following expression:
```
    pdf' = pdf * [ w * D + (1-w) * U ]
```
As a raccommendation, use interpolation only when the test statistic
distribution assumes unexpected shapes (i.e. peaks), otherwise keep the option
off.

### Related project

- [gerda-fitter](https://github.com/gipert/gerda-fitter)
