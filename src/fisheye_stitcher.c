#include "fisheye_stitcher.h"

FISHEYE fisheye[2];        // Input fisheye
PARAMS params;             // General parameters
BITMAP4 *spherical = NULL; // Output image

int main(void)
{
   // Transforms for fisheye1
   double fisheye_1_transform[][2] = {
       {YROLL, -90.0},
       {ZPAN, 1.7},
       {XTILT, 0.6},
   };
   int fisheye_1_transform_counter = sizeof(fisheye_1_transform) / sizeof(fisheye_1_transform[0]);

   // Transforms fro fisheteye2
   double fisheye_2_transform[][2] = {
       {XTILT, 0.0},
       {YROLL, 90},
       {ZPAN, 0.0},
   };
   int fisheye_2_transform_counter = sizeof(fisheye_2_transform) / sizeof(fisheye_2_transform[0]);

   // Create fisheye structures
   struct FISHEYE_PARAMS fisheye_1 = createFishEyeParams("sample.jpg", 1475, 1510, 1517, 191.5, 1, 1, fisheye_1_transform, fisheye_1_transform_counter);
   struct FISHEYE_PARAMS fisheye_2 = createFishEyeParams("sample.jpg", 1485, 4574, 1520, 194.7, 1, 1, fisheye_2_transform, fisheye_2_transform_counter);

   int optiterations = 1;
   bool debug = true;

   struct STITCHING_RESULT result = stitch(
       fisheye_1,
       fisheye_2,
       "./",
       2000,
       2,
       10,
       0,
       180,
       optiterations,
       10,
       20,
       5,
       true,
       debug);

   if (result.success)
   {
      fprintf(stderr, "Stitching succeeded! Output Message: %s\n Image: %s\n Optimized Image: %s\n",
              result.message, result.image, result.optimized_image);
      printOptimizedParams(result.optimized_params);
   }
   else
   {
      fprintf(stderr, "Stitching failed! Output Message: %s\n Image: %s\n", result.message, result.image);
   }

   return 0;
}

// Function to parse user input and create a list of transforms
TRANSFORM *create_transforms(double axis_values[][2], int count)
{
   // Allocate memory for the transforms array
   TRANSFORM *transforms = (TRANSFORM *)malloc(count * sizeof(TRANSFORM));

   // Populate the transforms array
   for (int i = 0; i < count; i++)
   {
      transforms[i].axis = axis_values[i][0];
      transforms[i].value = DTOR * axis_values[i][1];
      transforms[i].cvalue = cos(transforms[i].value);
      transforms[i].svalue = sin(transforms[i].value);
   }

   // Return the populated array
   return transforms;
}

void convert_transforms_to_array(const TRANSFORM *transforms, int transform_count, double array[][2], int *out_count)
{
   int count = 0;

   // Process the transforms up to the provided transform count
   for (int i = 0; i < transform_count && count < 10; i++)
   {
      array[count][0] = transforms[i].axis;
      array[count][1] = transforms[i].value / DTOR; // Convert back to degrees if needed
      count++;
   }

   // Set the output count
   *out_count = count;
}

/**
 * Create a FISHEYE_PARAMS structure.
 *
 * This function creates a FISHEYE_PARAMS structure from the given parameters.
 *
 * @param fname The filename of the fisheye image.
 * @param radius The radius of the fisheye image.
 * @param centerx The x coordinate of the center of the fisheye image.
 * @param centery The y coordinate of the center of the fisheye image.
 * @param fov The field of view of the fisheye image.
 * @param hflip Whether to flip the fisheye image horizontally.
 * @param vflip Whether to flip the fisheye image vertically.
 * @param transform_values An array of TRANSFORM structures, describing the transformations to apply to the fisheye image.
 * @param ntransform The number of TRANSFORM structures in the transform array.
 *
 * @return A FISHEYE_PARAMS structure, which contains the given parameters.
 */
struct FISHEYE_PARAMS createFishEyeParams(char fname[256], int radius, int centerx, int centery, double fov, int hflip, int vflip, double transform_values[][2], int ntransform)
{
   // Print ntransform to stderr
   fprintf(stderr, "ntransform: %d\n", ntransform);
   // Print the transform_values
   for (int i = 0; i < ntransform; i++)
   {
      fprintf(stderr, "Axis: %f, Value: %f\n", transform_values[i][0], transform_values[i][1]);
   }

   struct FISHEYE_PARAMS fisheye;
   strcpy(fisheye.fname, fname);
   fisheye.radius = radius;
   fisheye.centerx = centerx;
   fisheye.centery = centery;
   fisheye.fov = fov;
   fisheye.hflip = hflip;
   fisheye.vflip = vflip;
   fisheye.transform = create_transforms(transform_values, ntransform);
   fisheye.ntransform = ntransform;

   return fisheye;
}

/**
 * Create a STITCHING_RESULT structure with the given parameters.
 *
 * This function creates a STITCHING_RESULT structure from the given parameters.
 * The structure contains the success flag, a message string describing the
 * result of the stitching operation, the full path and filename of the output
 * image, the full path and filename of the optimized output image, and the
 * optimized parameters.
 *
 * @param success The success flag for the stitching operation.
 * @param message A message string describing the result of the stitching operation.
 * @param image The full path and filename of the output image.
 * @param optimized_image The full path and filename of the optimized output image.
 * @param optimized_params OPTIMIZED_PARAMS.
 *
 * @return A STITCHING_RESULT structure with the given parameters.
 */
struct STITCHING_RESULT createStitchResult(bool success, char message[512], char image[512], char optimized_image[512], struct OPTIMIZED_PARAMS optimized_params)
{
   struct STITCHING_RESULT result;

   // Set the success flag
   result.success = success;

   // Copy the message string
   strcpy(result.message, message);

   // Copy the full path and filename of the output image
   strcpy(result.image, image);

   // Copy the full path and filename of the optimized output image
   strcpy(result.optimized_image, optimized_image);

   // Copy the optimized parameters
   result.optimized_params = optimized_params;

   // Return the STITCHING_RESULT structure
   return result;
}

// Seed the random number generator at the beginning of the program
void initRandomNumberGenerator(void)
{
   srand((unsigned int)time(NULL));
}

char *generateRandomName(void)
{
   static char name[11]; // Static variable to hold the generated name
   snprintf(name, 11, "%04d_%04d", rand() % 10000, rand() % 10000);
   return name;
}

/**
 * Stitch two fisheye images together, using a blend zone.
 *
 * This function takes two fisheye parameters and a filename for the output image.
 * It returns a STITCHING_RESULT, which has a success flag and a message string.
 * If the success flag is true, the message string will contain the filename of
 * the output image.
 *
 * The function first populates the fisheye structures from the parameters, then
 * forms the spherical map by iterating over the output image and for each pixel
 * finding the corresponding pixel in the fisheye images and summing over the
 * supersampling set. The resulting image is then saved to the output filename.
 *
 * If the debug flag is set, the function will also write a parameter file, suitable
 * for normal dualfish2sphere usage, after each optimisation step.
 *
 * @param fisheye_1 The first fisheye parameters.
 * @param fisheye_2 The second fisheye parameters.
 * @param outwidth The width of the output image.
 * @param antialias The amount of antialiasing to use.
 * @param blendwidth The longitude for blending.
 * @param blendpower The power to use for the blend zone.
 * @param blendmid Mid point for blending
 * @param noptiterations The number of optimisation iterations to perform.
 * @param deltafov The amount to perturb the field of view for each optimisation iteration.
 * @param deltacenter The amount to perturb the center for each optimisation iteration.
 * @param deltatheta The amount to perturb the rotation for each optimisation iteration.
 * @param icorrection Whether to apply the inverse correction.
 * @param debug Whether to enable debugging output.
 * @return A STITCHING_RESULT, which has a success flag and a message string.
 */
struct STITCHING_RESULT stitch(
    struct FISHEYE_PARAMS fisheye_1, struct FISHEYE_PARAMS fisheye_2,
    char outputPath[512], int outwidth, int antialias,
    double blendwidth, double blendpower, double blendmid,
    int noptiterations,
    double deltafov, int deltacenter, double deltatheta,
    bool icorrection, bool debug)
{
   // Seed the random number generator at the beginning of the program
   initRandomNumberGenerator();
   int i, j, aj, ai, n = 0;
   int index, nantialias[2], inblendzone;
   char outfilename[256] = "\0";
   BITMAP4 black = {0, 0, 0, 255}, dev_color = {216, 250, 8, 255};
   double latitude0, longitude0, latitude, longitude;
   double weight = 1, blend = 1;
   COLOUR rgb, rgbsum[2], rgbzero = {0, 0, 0};
   double starttime = 0, stoptime = 0;
   int nopt;
   double fov[2];
   int centerx[2], centery[2];
   double r, theta, minerror = 1e32, opterror = 0, errorsum = 0;
   char fname[256];
   char optimized_image[512] = "";

   // Initial values for fisheye structure and general parameters
   InitParams();
   InitFisheye(&fisheye[0]);
   InitFisheye(&fisheye[1]);

   struct OPTIMIZED_PARAMS optimized_params_struct = (struct OPTIMIZED_PARAMS){
       .fishey1_params = fisheye_1,
       .fishey2_params = fisheye_2,
   };

   // Update the parameters from the function parameters
   // Outpup Filename
   strcpy(outfilename, generateRandomName());

   // width PARAM
   params.outwidth = outwidth;
   params.outwidth /= 4;
   params.outwidth *= 4;                   // Ensure multiple of 4
   params.outheight = params.outwidth / 2; // Default for equirectangular images, will be even
   // antialias PARAM
   params.antialias = antialias < 1 ? 1 : antialias;
   // blendwidth PARAM
   if ((params.blendwidth = DTOR * blendwidth) < 0)
      params.blendwidth = 0;
   params.blendwidth /= 2;
   // debug PARAM
   params.debug = debug;
   // blendpower PARAM
   params.blendpower = blendpower < 1 ? 1 : blendpower;
   // noptiterations PARAM
   noptiterations = noptiterations > 1 ? noptiterations : 1;
   // deltafov PARAM
   params.deltafov = DTOR * deltafov;
   // deltacenter PARAM
   params.deltacenter = deltacenter;
   // deltatheta PARAM
   params.deltatheta = DTOR * deltatheta;
   // icorrection PARAM
   params.icorrection = icorrection;
   // blendmid PARAM
   params.blendmid = blendmid * DTOR * 0.5;

   // Read parameter for fisheye_1
   if (!populateFisheyeStructure(0, fisheye_1.fname, fisheye_1.radius, fisheye_1.centerx, fisheye_1.centery, fisheye_1.fov, fisheye_1.hflip, fisheye_1.vflip, fisheye_1.transform, fisheye_1.ntransform))
   {
      return createStitchResult(false, "Failed to read image for fisheye 1", "", "", optimized_params_struct);
   }

   // Read parameter for fisheye_2
   if (!populateFisheyeStructure(1, fisheye_2.fname, fisheye_2.radius, fisheye_2.centerx, fisheye_2.centery, fisheye_2.fov, fisheye_2.hflip, fisheye_2.vflip, fisheye_2.transform, fisheye_2.ntransform))
   {
      return createStitchResult(false, "Failed to read image for fisheye 2", "", "", optimized_params_struct);
   }

   // Apply defaults and precompute values
   FisheyeDefaults(&fisheye[0]);
   FisheyeDefaults(&fisheye[1]);
   FlipFisheye(fisheye[0]);
   FlipFisheye(fisheye[1]);

   // Create output spherical (equirectangular) image
   spherical = Create_Bitmap(params.outwidth, params.outheight);
   if (spherical == NULL)
   {
      return createStitchResult(false, "Failed to create bitmap", "", "", optimized_params_struct);
   }

   // Must have blending on for optimisation
   if (noptiterations > 1 && params.blendwidth <= 0)
   {
      fprintf(stderr, "Warning: Must enable blending for optimisation, setting to 6 degrees\n");
      params.blendwidth = 3 * DTOR;
   }

   // Remember the baseline values
   for (j = 0; j < 2; j++)
   {
      fov[j] = fisheye[j].fov;
      centerx[j] = fisheye[j].centerx;
      centery[j] = fisheye[j].centery;
   }

   if (params.debug)
      DumpParameters();

   // Optimisation loop, otherwise just do once if noptiterations=1
   for (nopt = 0; nopt < noptiterations; nopt++)
   {
      // Only apply random perturbation after first iteration
      if (noptiterations > 1)
      {
         for (j = 0; j < 2; j++)
         {
            fisheye[j].fov = fov[j] + (drand48() - 0.5) * params.deltafov; // Half
            r = drand48() * params.deltacenter;
            theta = drand48() * TWOPI;
            fisheye[j].centerx = centerx[j] + r * cos(theta);
            fisheye[j].centery = centery[j] + r * sin(theta);
         }
         MakeRandomRotations();
      }

      if (noptiterations > 1 && nopt % (noptiterations / 100 == 0 ? 1 : noptiterations / 100) == 0)
         fprintf(stderr, "Optimisation step %8d of %8d\n", nopt, noptiterations);

      opterror = 0;
      errorsum = 0;

      // Form the spherical map
      starttime = GetTime();
      Erase_Bitmap(spherical, params.outwidth, params.outheight, black);
      for (j = 0; j < params.outheight; j++)
      {
         latitude0 = PI * j / (double)params.outheight - PID2; // -pi/2 ... pi/2

         for (i = 0; i < params.outwidth; i++)
         {
            longitude0 = TWOPI * i / (double)params.outwidth - PI; // -pi ... pi

            // Blending masks, only depend on longitude
            if (params.blendwidth > 0)
            {
               blend = (params.blendmid + params.blendwidth - fabs(longitude0)) / (2 * params.blendwidth); // 0 ... 1
               if (blend < 0)
                  blend = 0;
               if (blend > 1)
                  blend = 1;
               if (params.blendpower > 1)
               {
                  blend = 2 * blend - 1; // -1 to 1
                  blend = 0.5 + 0.5 * SIGN(blend) * pow(fabs(blend), 1.0 / params.blendpower);
               }
            }
            else
            { // No blend
               blend = 0;
               if (ABS(longitude0) <= params.blendmid) // Hard edge
                  blend = 1;
            }

            // Are we in the blending zones
            inblendzone = FALSE;
            if (longitude0 <= params.blendmid + params.blendwidth && longitude0 >= params.blendmid - params.blendwidth)
               inblendzone = TRUE;
            if (longitude0 >= -params.blendmid - params.blendwidth && longitude0 <= -params.blendmid + params.blendwidth)
               inblendzone = TRUE;

            // If optimising then only need to calculate image within the blend zone
            if (noptiterations > 1 && !inblendzone)
               continue;

            // Initialise antialiasing accumulation variables
            for (n = 0; n < 2; n++)
            {
               rgbsum[n] = rgbzero;
               nantialias[n] = 0;
            }

            // Antialiasing, inner loops
            // Find the corresponding pixel in the fisheye image
            // Sum over the supersampling set
            for (ai = 0; ai < params.antialias; ai++)
            {
               longitude = longitude0 + ai * TWOPI / (params.antialias * params.outwidth);
               for (aj = 0; aj < params.antialias; aj++)
               {
                  latitude = latitude0 + aj * M_PI / (params.antialias * params.outheight);
                  for (n = 0; n < 2; n++)
                  {
                     if (FindFishPixel(n, latitude, longitude, &rgb))
                     {
                        rgbsum[n].r += rgb.r;
                        rgbsum[n].g += rgb.g;
                        rgbsum[n].b += rgb.b;
                        nantialias[n]++;
                     }
                  }
               } // aj
            } // ai

            // Normalise by antialiasing samples
            for (n = 0; n < 2; n++)
            {
               if (nantialias[n] > 0)
               {
                  rgbsum[n].r /= nantialias[n];
                  rgbsum[n].g /= nantialias[n];
                  rgbsum[n].b /= nantialias[n];
               }
            }

            // Update antialiased value to final image with blending
            index = j * params.outwidth + i;
            spherical[index].r = blend * rgbsum[0].r + (1 - blend) * rgbsum[1].r;
            spherical[index].g = blend * rgbsum[0].g + (1 - blend) * rgbsum[1].g;
            spherical[index].b = blend * rgbsum[0].b + (1 - blend) * rgbsum[1].b;

            // Determine error metric if in optimisation mode
            // Experimental, weight higher if closer to the center of blend
            if (noptiterations > 1 && inblendzone)
            {
               // weight = 1;
               weight = 1 - 2 * fabs(0.5 - blend); // 0 to 1 in middle of blend to 0
               if (j > 0.2 * params.outheight && j < 0.8 * params.outheight)
               {
                  opterror += CalcError(rgbsum[0], rgbsum[1], weight);
                  errorsum += weight;
               }
            }
         } // i
      } // j
      stoptime = GetTime();

      // Write a parameter file, suitable for normal dualfish2sphere usage
      opterror /= errorsum; // Normalise to "per pixel"
      if (noptiterations > 1 && opterror < minerror)
      {
         int fisheye_1_transform_count;
         int fisheye_2_transform_count;
         double fisheye_1_transform[fisheye[0].ntransform][2];
         double fisheye_2_transform[fisheye[1].ntransform][2];
         convert_transforms_to_array(fisheye[0].transform, fisheye[0].ntransform, fisheye_1_transform, &fisheye_1_transform_count);
         convert_transforms_to_array(fisheye[1].transform, fisheye[1].ntransform, fisheye_2_transform, &fisheye_2_transform_count);

         // Update optimized params with the new values
         optimized_params_struct = (struct OPTIMIZED_PARAMS){
             .fishey1_params = createFishEyeParams(fisheye[0].fname, fisheye[0].radius, fisheye[0].centerx, fisheye[0].height - 1 - fisheye[0].centery, fisheye[0].fov * 2 * RTOD, fisheye[0].hflip, fisheye[0].vflip, fisheye_1_transform, fisheye_1_transform_count),
             .fishey2_params = createFishEyeParams(fisheye[1].fname, fisheye[1].radius, fisheye[1].centerx, fisheye[1].height - 1 - fisheye[1].centery, fisheye[1].fov * 2 * RTOD, fisheye[1].hflip, fisheye[1].vflip, fisheye_2_transform, fisheye_2_transform_count),
         };

         // Write image so far, will just be the blend strip
         sprintf(fname, "%s_%s", StripExtension(outfilename), "optimized");
         sprintf(optimized_image, "%s/%s", outputPath, AddExtension(fname));
         WriteOutputImage(fname, outputPath);
         fprintf(stderr, "Optimisation step %8d of %8d Error: %5.1lf ", nopt, noptiterations, opterror);
         minerror = opterror; // Remember best so far
      }

   } // nopt

   // Timing and optionally show the blend range
   if (params.debug)
   {
      fprintf(stderr, "Time for sampling: %g\n", stoptime - starttime);
      for (j = 0; j < params.outheight; j++)
      {
         i = params.outwidth / 4.0;
         Draw_Pixel(spherical, params.outwidth, params.outheight, i, j, dev_color);
         i = 3 * params.outwidth / 4.0;
         Draw_Pixel(spherical, params.outwidth, params.outheight, i, j, dev_color);
         if (params.blendwidth > 0)
         {
            i = params.outwidth / 4.0 + params.outwidth * params.blendwidth / TWOPI;
            Draw_Pixel(spherical, params.outwidth, params.outheight, i, j, dev_color);
            i = params.outwidth / 4.0 - params.outwidth * params.blendwidth / TWOPI;
            Draw_Pixel(spherical, params.outwidth, params.outheight, i, j, dev_color);
            i = 3 * params.outwidth / 4.0 + params.outwidth * params.blendwidth / TWOPI;
            Draw_Pixel(spherical, params.outwidth, params.outheight, i, j, dev_color);
            i = 3 * params.outwidth / 4.0 - params.outwidth * params.blendwidth / TWOPI;
            Draw_Pixel(spherical, params.outwidth, params.outheight, i, j, dev_color);
         }
      }
   }

   // Save final output
   if (WriteOutputImage(outfilename, outputPath))
   {
      char fullpath[512];
      sprintf(fullpath, "%s/%s", outputPath, AddExtension(outfilename));
      return createStitchResult(true, "Image stitched successfully", fullpath, optimized_image, optimized_params_struct);
   }
   else
   {
      return createStitchResult(false, "Failed creating output image", "", "", optimized_params_struct);
   }
}

/**
 * Initialize a FISHEYE structure with default values.
 *
 * @param f Pointer to the FISHEYE structure to initialize.
 *
 */
void InitFisheye(FISHEYE *f)
{
   f->fname[0] = '\0';
   f->image = NULL;
   f->width = 0;
   f->height = 0;
   f->centerx = -1;
   f->centery = -1;
   f->radius = -1;
   f->fov = 180;
   f->hflip = 1;
   f->vflip = 1;
   f->transform = NULL;
   f->ntransform = 0;
}

/**
 * Sets default values for a FISHEYE structure.
 *
 * @param f Pointer to the FISHEYE structure to set defaults for.
 *
 * This function sets the following default values for a FISHEYE structure:
 * - fov is divided by 2, converted to radians
 * - center is set to the image center if not already set
 * - origin is set to bottom left
 * - radius is set to half height if not already set
 * - precomputes sine and cosine of transformation angles
 */
void FisheyeDefaults(FISHEYE *f)
{
   int j;

   // fov will only be used as half value, and radians
   f->fov /= 2;
   f->fov *= DTOR;

   // Set center to image center, if not set in parameter file
   if (f->centerx < 0 || f->centery < 0)
   {
      f->centerx = f->width / 2;
      f->centery = f->height / 2;
   }

   // Origin bottom left
   f->centery = f->height - 1 - f->centery;

   // Set fisheye radius to half height, if not set in parameter file
   if (f->radius < 0)
      f->radius = f->height / 2;

   // Precompute sine and cosine of transformation angles
   for (j = 0; j < f->ntransform; j++)
   {
      f->transform[j].cvalue = cos(f->transform[j].value);
      f->transform[j].svalue = sin(f->transform[j].value);
   }
}

/**
 * Given a longitude and latitude, calculate the rgb value from the fisheye
 *
 * @param n The index of the fisheye image (0 or 1)
 * @param latitude The latitude of the pixel in radians
 * @param longitude The longitude of the pixel in radians
 * @param rgb Pointer to COLOUR structure to store the calculated rgb value
 *
 * @return TRUE if the pixel is inside the fisheye image, FALSE otherwise
 *
 * This function calculates the rgb value of a pixel on the fisheye image
 * given its latitude and longitude. The function takes the index of the fisheye
 * image, the latitude and longitude of the pixel in radians, and a pointer to
 * a COLOUR structure to store the calculated rgb value. The function returns
 * TRUE if the pixel is inside the fisheye image, and FALSE otherwise.
 */
int FindFishPixel(int n, double latitude, double longitude, COLOUR *rgb)
{
   int k, index;
   COLOUR c;
   HSV hsv;
   XYZ p, q = {0, 0, 0};
   double theta, phi, r;
   int u, v;

   // Ignore pixels that will never be touched because out of blend range
   if (n == 0)
   {
      if (longitude > params.blendmid + params.blendwidth || longitude < -params.blendmid - params.blendwidth)
         return (FALSE);
   }
   if (n == 1)
   {
      if (longitude > -params.blendmid + params.blendwidth && longitude < params.blendmid - params.blendwidth)
         return (FALSE);
   }

   // Turn by 180 degrees for the second fisheye
   if (n == 1)
   {
      longitude += M_PI;
   }

   // p is the ray from the camera position into the scene
   p.x = cos(latitude) * sin(longitude);
   p.y = cos(latitude) * cos(longitude);
   p.z = sin(latitude);

   // Apply fisheye correction transformation
   for (k = 0; k < fisheye[n].ntransform; k++)
   {
      switch (fisheye[n].transform[k].axis)
      {
      case XTILT:
         q.x = p.x;
         q.y = p.y * fisheye[n].transform[k].cvalue + p.z * fisheye[n].transform[k].svalue;
         q.z = -p.y * fisheye[n].transform[k].svalue + p.z * fisheye[n].transform[k].cvalue;
         break;
      case YROLL:
         q.x = p.x * fisheye[n].transform[k].cvalue + p.z * fisheye[n].transform[k].svalue;
         q.y = p.y;
         q.z = -p.x * fisheye[n].transform[k].svalue + p.z * fisheye[n].transform[k].cvalue;
         break;
      case ZPAN:
         q.x = p.x * fisheye[n].transform[k].cvalue + p.y * fisheye[n].transform[k].svalue;
         q.y = -p.x * fisheye[n].transform[k].svalue + p.y * fisheye[n].transform[k].cvalue;
         q.z = p.z;
         break;
      }
      p = q;
   }

   // Calculate fisheye coordinates
   theta = atan2(p.z, p.x);
   phi = atan2(sqrt(p.x * p.x + p.z * p.z), p.y);
   r = phi / fisheye[n].fov; // 0 ... 1

   // Determine the u,v coordinate
   u = fisheye[n].centerx + fisheye[n].radius * r * cos(theta);
   if (u < 0 || u >= fisheye[n].width)
      return (FALSE);
   v = fisheye[n].centery + fisheye[n].radius * r * sin(theta);
   if (v < 0 || v >= fisheye[n].height)
      return (FALSE);
   index = v * fisheye[n].width + u;
   c.r = fisheye[n].image[index].r;
   c.g = fisheye[n].image[index].g;
   c.b = fisheye[n].image[index].b;

   // Intensity correction
   if (params.icorrection)
   {
      c.r /= 255.0;
      c.g /= 255.0;
      c.b /= 255.0;
      hsv = RGB2HSV(c);
      hsv.v *= (params.ifcn[0] + r * (params.ifcn[1] + r * (params.ifcn[2] + r * (params.ifcn[3] + r * (params.ifcn[4] + r * params.ifcn[5])))));
      if (hsv.v < 0)
         hsv.v = 0;
      if (hsv.v > 1)
         hsv.v = 1;
      c = HSV2RGB(hsv);
      c.r *= 255.0;
      c.g *= 255.0;
      c.b *= 255.0;
   }

   rgb->r = c.r;
   rgb->g = c.g;
   rgb->b = c.b;

   return (TRUE);
}

/*
   Purely for debugging
*/
void DumpParameters(void)
{
   int i, j;

   for (i = 0; i < 2; i++)
   {
      fprintf(stderr, "FISHEYE: %d\n", i);
      fprintf(stderr, "   IMAGE: %s (%dx%d)\n", fisheye[i].fname, fisheye[i].width, fisheye[i].height);
      fprintf(stderr, "   CENTER: %d,%d\n", fisheye[i].centerx, fisheye[i].height - 1 - fisheye[i].centery);
      fprintf(stderr, "   RADIUS: %d\n", fisheye[i].radius);
      fprintf(stderr, "   FOV: %g\n", fisheye[i].fov * 2 * RTOD);
      fprintf(stderr, "   HFLIP: %d\n", fisheye[i].hflip);
      fprintf(stderr, "   VFLIP: %d\n", fisheye[i].vflip);
      for (j = 0; j < fisheye[i].ntransform; j++)
      {
         if (fisheye[i].transform[j].axis == XTILT)
            fprintf(stderr, "   ROTATEX: ");
         else if (fisheye[i].transform[j].axis == YROLL)
            fprintf(stderr, "   ROTATEY: ");
         else if (fisheye[i].transform[j].axis == ZPAN)
            fprintf(stderr, "   ROTATEZ: ");
         fprintf(stderr, "%.1lf\n", fisheye[i].transform[j].value * RTOD);
      }
   }
}

// Function to print FISHEYE_PARAMS
void printFisheyeParams(struct FISHEYE_PARAMS fisheye_params)
{
   int i;
   fprintf(stderr, "Fisheye Params:\n");
   fprintf(stderr, "   Filename: %s\n", fisheye_params.fname);
   fprintf(stderr, "   Radius: %d\n", fisheye_params.radius);
   fprintf(stderr, "   Center: %d,%d\n", fisheye_params.centerx, fisheye_params.centery);
   fprintf(stderr, "   Field of View (FOV): %f\n", fisheye_params.fov * 2 * RTOD);
   fprintf(stderr, "   Horizontal Flip: %d\n", fisheye_params.hflip);
   fprintf(stderr, "   Vertical Flip: %d\n", fisheye_params.vflip);

   for (i = 0; i < fisheye_params.ntransform; i++)
   {
      switch (fisheye_params.transform[i].axis)
      {
      case XTILT:
         fprintf(stderr, "   ROTATEX: %.1lf\n", fisheye_params.transform[i].value * RTOD);
         break;
      case YROLL:
         fprintf(stderr, "   ROTATEY: %.1lf\n", fisheye_params.transform[i].value * RTOD);
         break;
      case ZPAN:
         fprintf(stderr, "   ROTATEZ: %.1lf\n", fisheye_params.transform[i].value * RTOD);
         break;
      }
   }
}

// Function to print OPTIMIZED_PARAMS
void printOptimizedParams(struct OPTIMIZED_PARAMS optimized_params)
{
   fprintf(stderr, "Optimized Params:\n");

   // Print fisheye1_params
   fprintf(stderr, "Fisheye 1:\n");
   printFisheyeParams(optimized_params.fishey1_params);

   // Print fisheye2_params
   fprintf(stderr, "Fisheye 2:\n");
   printFisheyeParams(optimized_params.fishey2_params);
}

/**
 * Populate a FISHEYE structure from the given parameters.
 *
 * This function populates a FISHEYE structure from the given parameters, and
 * reads the image file given by fname into memory.
 *
 * @param nfish The index of the FISHEYE structure to populate.
 * @param fname The filename of the fisheye image.
 * @param radius The radius of the fisheye image.
 * @param centerx The x coordinate of the center of the fisheye image.
 * @param centery The y coordinate of the center of the fisheye image.
 * @param fov The field of view of the fisheye image.
 * @param hflip Whether to flip the fisheye image horizontally.
 * @param vflip Whether to flip the fisheye image vertically.
 * @param transform An array of TRANSFORM structures, describing the transformations to apply to the fisheye image.
 * @param ntransform The number of TRANSFORM structures in the transform array.
 *
 * @return TRUE if the image was read successfully, FALSE otherwise.
 */
int populateFisheyeStructure(int nfish, char fname[256], int radius, int centerx, int centery, double fov, int hflip, int vflip, TRANSFORM *transform, int ntransform)
{
   int w, h, d;
   FILE *fimg;
   // Check if is an image
   if (IsJPEG(fname))
      params.fileformat = JPG;
   else
      params.fileformat = TGA;
   if ((fimg = fopen(fname, "rb")) == NULL)
   {
      fprintf(stderr, "   Failed to open image file \"%s\"\n", fname);
      return (FALSE);
   }
   // Get image size depending on format
   if (params.fileformat == JPG)
      JPEG_Info(fimg, &fisheye[nfish].width, &fisheye[nfish].height, &d);
   else
      TGA_Info(fimg, &fisheye[nfish].width, &fisheye[nfish].height, &d);
   // Create image
   fisheye[nfish].image = Create_Bitmap(fisheye[nfish].width, fisheye[nfish].height);
   if (params.fileformat == JPG)
   {
      if (JPEG_Read(fimg, fisheye[nfish].image, &w, &h) != 0)
      {
         fprintf(stderr, "   Failed to correctly read image \"%s\"\n", fname);
         return (FALSE);
      }
   }
   else
   {
      if (TGA_Read(fimg, fisheye[nfish].image, &w, &h) != 0)
      {
         fprintf(stderr, "   Failed to correctly read image \"%s\"\n", fname);
         return (FALSE);
      }
   }
   fclose(fimg);
   // Radius
   fisheye[nfish].radius = radius;
   // Center point
   fisheye[nfish].centerx = centerx;
   fisheye[nfish].centery = centery;
   // FOV
   fisheye[nfish].fov = fov;
   // Flip
   if (hflip < 0)
      fisheye[nfish].hflip = -1;
   else
      fisheye[nfish].hflip = 1;
   // VFlip
   if (vflip < 0)
      fisheye[nfish].vflip = -1;
   else
      fisheye[nfish].vflip = 1;

   fisheye[nfish].transform = transform;
   fisheye[nfish].ntransform = ntransform;
   return (TRUE);
}

char *StripExtension(char *s)
{
   int i;

   // remove extension
   for (i = (int)(strlen(s) - 1); i > 0; i--)
   {
      if (s[i] == '.')
      {
         s[i] = '\0';
         break;
      }
   }
   return s;
}

char *AddExtension(char *s)
{
   // Add extension
   if (params.fileformat == JPG)
      strcat(s, ".jpg");
   else
      strcat(s, ".tga");

   return s;
}

int WriteOutputImage(char *s, char *outputpath)
{
   FILE *fptr;
   char fname[512];
   char fullpath[512];
   // Strip extension if provided
   strcpy(fname, StripExtension(s));

   // Add extension to fname
   AddExtension(fname);

   // Create the full path
   sprintf(fullpath, "%s/%s", outputpath, fname);

   fprintf(stderr, "Output file  \"%s\"\n", fullpath);

   // Open file
   if ((fptr = fopen(fullpath, "wb")) == NULL)
   {
      fprintf(stderr, "Failed to create empty output file \"%s\"\n", fullpath);
      fprintf(stderr, "Error: %s (errno: %d)\n", strerror(errno), errno);
      return (FALSE);
   }

   // Write image
   if (params.fileformat == JPG)
      JPEG_Write(fptr, spherical, params.outwidth, params.outheight, 100);
   else
      Write_Bitmap(fptr, spherical, params.outwidth, params.outheight, 12);

   fclose(fptr);
   return (TRUE);
}

/*
   This should be a metric for best fit, smaller the better
*/
double CalcError(COLOUR rgb1, COLOUR rgb2, double weight)
{
   double error;
   COLOUR dc;

   // This is root-mean-square error
   dc.r = rgb1.r - rgb2.r;
   dc.g = rgb1.g - rgb2.g;
   dc.b = rgb1.b - rgb2.b;
   error = dc.r * dc.r + dc.g * dc.g + dc.b * dc.b;
   error *= weight;

   return (error);
}

/*
   Add 3 random rotations angles for camera 0 only
   Don't need to apply to both cameras, preserve any user transforms for camera 1
*/
void MakeRandomRotations(void)
{
   int j;
   static int first = TRUE;
   static int nt;

   if (first)
   {
      nt = fisheye[0].ntransform;
      fisheye[0].transform = realloc(fisheye[0].transform, (nt + 3) * sizeof(TRANSFORM));
      fisheye[0].ntransform += 3;
      first = FALSE;
   }

   // Try each of the 6 possible transformation orders
   switch (rand() % 6)
   {
   case 0:
      fisheye[0].transform[nt + 0].axis = XTILT;
      fisheye[0].transform[nt + 1].axis = YROLL;
      fisheye[0].transform[nt + 2].axis = ZPAN;
      break;
   case 1:
      fisheye[0].transform[nt + 0].axis = XTILT;
      fisheye[0].transform[nt + 1].axis = ZPAN;
      fisheye[0].transform[nt + 2].axis = YROLL;
      break;
   case 2:
      fisheye[0].transform[nt + 0].axis = YROLL;
      fisheye[0].transform[nt + 1].axis = ZPAN;
      fisheye[0].transform[nt + 2].axis = XTILT;
      break;
   case 3:
      fisheye[0].transform[nt + 0].axis = YROLL;
      fisheye[0].transform[nt + 1].axis = XTILT;
      fisheye[0].transform[nt + 2].axis = ZPAN;
      break;
   case 4:
      fisheye[0].transform[nt + 0].axis = ZPAN;
      fisheye[0].transform[nt + 1].axis = XTILT;
      fisheye[0].transform[nt + 2].axis = YROLL;
      break;
   case 5:
      fisheye[0].transform[nt + 0].axis = ZPAN;
      fisheye[0].transform[nt + 1].axis = YROLL;
      fisheye[0].transform[nt + 2].axis = XTILT;
      break;
   }

   // Precompute sine and cosine of angles for the new transforms
   for (j = nt; j < fisheye[0].ntransform; j++)
   {
      fisheye[0].transform[j].value = 2 * (drand48() - 0.5) * params.deltatheta;
      fisheye[0].transform[j].cvalue = cos(fisheye[0].transform[j].value);
      fisheye[0].transform[j].svalue = sin(fisheye[0].transform[j].value);
   }
}

/*
   Calculate HSV from RGB
   Hue is in degrees
   Value is betweeen 0 and 1
   Saturation is between 0 and 1
*/
HSV RGB2HSV(COLOUR c1)
{
   double themin, themax, delta;
   HSV c2;

   themin = MIN(c1.r, MIN(c1.g, c1.b));
   themax = MAX(c1.r, MAX(c1.g, c1.b));
   delta = themax - themin;
   c2.v = themax;
   c2.s = 0;
   if (themax > 0)
      c2.s = delta / themax;
   c2.h = 0;
   if (delta > 0)
   {
      if (themax == c1.r && themax != c1.g)
         c2.h += (c1.g - c1.b) / delta;
      if (themax == c1.g && themax != c1.b)
         c2.h += (2 + (c1.b - c1.r) / delta);
      if (themax == c1.b && themax != c1.r)
         c2.h += (4 + (c1.r - c1.g) / delta);
      c2.h *= 60;
      if (c2.h < 0)
         c2.h += 360;
   }
   return (c2);
}

/*
   Calculate RGB from HSV, reverse of RGB2HSV()
   Hue is in degrees
   Value is between 0 and 1
   Saturation is between 0 and 1
*/
COLOUR HSV2RGB(HSV c1)
{
   COLOUR c2, sat;

   while (c1.h < 0)
      c1.h += 360;
   while (c1.h > 360)
      c1.h -= 360;

   if (c1.h < 120)
   {
      sat.r = (120 - c1.h) / 60.0;
      sat.g = c1.h / 60.0;
      sat.b = 0;
   }
   else if (c1.h < 240)
   {
      sat.r = 0;
      sat.g = (240 - c1.h) / 60.0;
      sat.b = (c1.h - 120) / 60.0;
   }
   else
   {
      sat.r = (c1.h - 240) / 60.0;
      sat.g = 0;
      sat.b = (360 - c1.h) / 60.0;
   }
   sat.r = MIN(sat.r, 1);
   sat.g = MIN(sat.g, 1);
   sat.b = MIN(sat.b, 1);

   c2.r = (1 - c1.s + c1.s * sat.r) * c1.v;
   c2.g = (1 - c1.s + c1.s * sat.g) * c1.v;
   c2.b = (1 - c1.s + c1.s * sat.b) * c1.v;

   return (c2);
}

void InitParams(void)
{
   time_t secs;
   int seed;

   params.debug = FALSE;
   params.antialias = 2;               // Supersampling antialising
   params.blendmid = 180 * DTOR * 0.5; // Mid point for blending
   params.blendwidth = 0;              // Angular blending width
   params.blendpower = 1;
   params.outwidth = 4096;
   params.outheight = 2048;

   // Intensity correction
   // Coefficients of 5th order polynomial brightness correction
   // 1 + a[1]x + a[2]x^2 + a[3]x^3 + a[4]x^4 + a[5]x^5
   // Should map from 0 to 1 to 0 to generally 1+delta
   params.icorrection = FALSE; // Perform intensity correction or not
   // double ifcn[6] = {0.9998,0.0459,-0.5894,2.4874,-4.2037,2.4599}; // rises to 1.2
   // double ifcn[6] = {1.0,0.1,-1.0417,3.6458,-5.2083,2.6042}; // rises to 1.1
   // double ifcn[6] = {1.0,0.05,-0.5208,1.8229,-2.6042,1.3021}; // rises to 1.05
   params.ifcn[0] = 1.0;
   params.ifcn[1] = 0.1;
   params.ifcn[2] = -1.0417;
   params.ifcn[3] = 3.6458;
   params.ifcn[4] = -5.2083;
   params.ifcn[5] = 2.6042;

   // Experimental optimisation
   params.deltafov = 10 * DTOR;  // Variation of fov
   params.deltacenter = 20;      // Variation of fisheye center coordinates
   params.deltatheta = 5 * DTOR; // Variation of rotations

   params.fileformat = TGA;

   // Random number seed
   time(&secs);
   seed = (int)secs;
   // seed = 12345; // Use for constant seed
   srand48(seed);
}

void FlipFisheye(FISHEYE f)
{
   BITMAP4 c1, c2, black = {0, 0, 0};
   int i, j, x1, y1, x2, y2;
   int index1, index2;

   if (f.hflip < 0)
   {
      for (i = 1; i <= f.radius; i++)
      {
         for (j = -f.radius; j <= f.radius; j++)
         {
            x1 = f.centerx - i;
            y1 = f.centery + j;
            x2 = f.centerx + i;
            y2 = f.centery + j;
            index1 = y1 * f.width + x1;
            index2 = y2 * f.width + x2;

            if (x1 < 0 || y1 < 0 || x1 >= f.width || y1 >= f.height)
               c1 = black;
            else
               c1 = f.image[index1];
            if (x2 < 0 || y2 < 0 || x2 >= f.width || y2 >= f.height)
               c2 = black;
            else
               c2 = f.image[index2];

            if (x1 >= 0 || y1 >= 0 || x1 < f.width || y1 < f.height)
               f.image[index1] = c2;
            if (x2 >= 0 || y2 >= 0 || x2 < f.width || y2 < f.height)
               f.image[index2] = c1;
         }
      }
   }

   if (f.vflip < 0)
   {
      for (i = -f.radius; i <= f.radius; i++)
      {
         for (j = 1; j <= f.radius; j++)
         {
            x1 = f.centerx + i;
            y1 = f.centery - j;
            x2 = f.centerx + i;
            y2 = f.centery + j;
            index1 = y1 * f.width + x1;
            index2 = y2 * f.width + x2;

            if (x1 < 0 || y1 < 0 || x1 >= f.width || y1 >= f.height)
               c1 = black;
            else
               c1 = f.image[index1];
            if (x2 < 0 || y2 < 0 || x2 >= f.width || y2 >= f.height)
               c2 = black;
            else
               c2 = f.image[index2];

            if (x1 >= 0 || y1 >= 0 || x1 < f.width || y1 < f.height)
               f.image[index1] = c2;
            if (x2 >= 0 || y2 >= 0 || x2 < f.width || y2 < f.height)
               f.image[index2] = c1;
         }
      }
   }
}

XYZ RotateX(XYZ p, double theta)
{
   XYZ q;

   q.x = p.x;
   q.y = p.y * cos(theta) + p.z * sin(theta);
   q.z = -p.y * sin(theta) + p.z * cos(theta);
   return (q);
}

XYZ RotateY(XYZ p, double theta)
{
   XYZ q;

   q.x = p.x * cos(theta) - p.z * sin(theta);
   q.y = p.y;
   q.z = p.x * sin(theta) + p.z * cos(theta);
   return (q);
}

XYZ RotateZ(XYZ p, double theta)
{
   XYZ q;

   q.x = p.x * cos(theta) + p.y * sin(theta);
   q.y = -p.x * sin(theta) + p.y * cos(theta);
   q.z = p.z;
   return (q);
}

#ifdef _WIN32 // This macro is defined by MSVC, MinGW, etc., on Windows
#include <windows.h>

double GetTime(void)
{
   LARGE_INTEGER frequency;
   LARGE_INTEGER counter;

   QueryPerformanceFrequency(&frequency);
   QueryPerformanceCounter(&counter);

   return (double)counter.QuadPart / (double)frequency.QuadPart;
}

#else // Assume POSIX-compliant system
#include <sys/time.h>

double GetTime(void)
{
   double sec = 0;
   struct timeval tp;

   gettimeofday(&tp, NULL);
   sec = tp.tv_sec + tp.tv_usec / 1000000.0;

   return sec;
}

#endif
