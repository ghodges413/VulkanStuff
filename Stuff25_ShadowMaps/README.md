# Shadow Maps
![Shadow Maps](screenshot.jpg)

Shadow mapping is probably the most popular way to add shadowed lighting to a renderer.  In the late 90's and early 2000's stencil shadows were also popular, but they have since fallen out of favor.

Shadow mapping works by rendering the scene from the lights perspective and storing the depths into a depth map that is specific to the light.  Then when the light is rendered into the final scene, it checks the light's depth map to determine if a particular fragment is in shadow or should be lit.