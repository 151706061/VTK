[![Documentation Status](https://readthedocs.org/projects/fides/badge/?version=latest)](https://fides.readthedocs.io/en/latest/?badge=latest)

## Fides: Adaptable Data Interfaces and Services

Fides is a library for adding metadata to ADIOS2 files so they can be more easily read and understood by Viskores.

The metadata required to visualize a dataset is often different than the metadata required in other contexts, so despite the fact we want our simulation data to be "self-describing", this statement has different meanings in different contexts, and hence it is almost impossible to realize in practice.

To use Fides, you must first create a `.json` file which has information relevant for processing the file in Viskores.

## Documentation

Documentation is hosted at [Read The Docs](https://fides.readthedocs.io/en/latest/?badge=latest).

## Dependencies

Fides depends of Viskores and ADIOS2:

- For Viskores, we require any version equal or higher than 1.0.
- For ADIOS2, we recommend v2.8.0 or later. Run the unit tests if doubt arises.

## Testing

`Fides` uses `git lfs` to manage its datasets.
This must be initialized after cloning.

```
fides$ git lfs install
fides$ git lfs pull
```

The tests are managed by `ctest`, so in the build directory just run

```
build_fides$ ctest -V
```
