### Path Tracer
This is a backwards unidirectional path tracer. It currently only supports lambertian surfaces.
It uses importance sampling for area light sources to reduce noiser per sample.

This is a toy path tracer. It doesn't feature the following:
- Anti aliasing*
- Arbitrary light shapes
- Arbitrary light directions*
- Accurate camera abstraction
- Motion blurring*
- Volumetric lighting
- Image based lighting
- Transparent materials
- Refractive materials
- Point lights*

Some of these features are not grounded on difficult mathematical concepts. Nevertheless, implementing them
into the demo requires architectural changes that I'm unable to evaluate currently.

'These features are relatively easy to implement. I will add them ine future if I get to it.

### Model loading
The demo requires a .mof file to be loaded. It is a special file that contains information about the geometry
and materials of a model. The .mof file is created with an external tool, that I will add to the project
in the near future.

Known bugs:
- Loading in a new asset does not work for the CPU tracer. This is probably related to a mistake in the usage
of DX12 rather than in the way .mof files are handled.
