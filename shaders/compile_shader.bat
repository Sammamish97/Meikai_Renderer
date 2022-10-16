fxc /Od /Zi /T vs_5_1 /E VS /enable_unbounded_descriptor_tables /Fo GeomVS.cso GeometryPass.hlsl
fxc /Od /Zi /T ps_5_1 /E PS /enable_unbounded_descriptor_tables /Fo GeomPS.cso GeometryPass.hlsl

fxc /Od /Zi /T vs_5_1 /E VS /enable_unbounded_descriptor_tables /Fo ScreenQuadVS.cso ScreenQuad.hlsl
fxc /Od /Zi /T ps_5_1 /E PS /enable_unbounded_descriptor_tables /Fo LightingPS.cso LightingPass.hlsl

fxc /Od /Zi /T ps_5_1 /E PS /enable_unbounded_descriptor_tables /Fo SsaoPS.cso SsaoPass.hlsl

fxc /Od /Zi /T vs_5_1 /E VS /enable_unbounded_descriptor_tables /Fo DefaultForwardVS.cso DefaultForward.hlsl
fxc /Od /Zi /T ps_5_1 /E PS /enable_unbounded_descriptor_tables /Fo DefaultForwardPS.cso DefaultForward.hlsl

fxc /Od /Zi /T vs_5_1 /E VS /enable_unbounded_descriptor_tables /Fo DebugJointVS.cso DebugJoint.hlsl
fxc /Od /Zi /T ps_5_1 /E PS /enable_unbounded_descriptor_tables /Fo DebugJointPS.cso DebugJoint.hlsl

fxc /Od /Zi /T vs_5_1 /E VS /enable_unbounded_descriptor_tables /Fo SkeletalGeomVS.cso SkeletalGeometryPass.hlsl
fxc /Od /Zi /T ps_5_1 /E PS /enable_unbounded_descriptor_tables /Fo SkeletalGeomPS.cso SkeletalGeometryPass.hlsl

fxc /Od /Zi /T vs_5_1 /E VS /enable_unbounded_descriptor_tables /Fo SkyboxVS.cso SkyboxPass.hlsl
fxc /Od /Zi /T ps_5_1 /E PS /enable_unbounded_descriptor_tables /Fo SkyboxPS.cso SkyboxPass.hlsl

fxc /Od /Zi /T vs_5_1 /E VS /enable_unbounded_descriptor_tables /Fo ShadowVS.cso ShadowPass.hlsl
fxc /Od /Zi /T ps_5_1 /E PS /enable_unbounded_descriptor_tables /Fo ShadowPS.cso ShadowPass.hlsl

fxc /Od /Zi /T cs_5_1 /E HorzBlurCS /enable_unbounded_descriptor_tables /Fo HBlurCS.cso Blur.hlsl
fxc /Od /Zi /T cs_5_1 /E VertBlurCS /enable_unbounded_descriptor_tables /Fo VBlurCS.cso Blur.hlsl

fxc /Od /Zi /T cs_5_1 /E EquiRectToCubemapCS /enable_unbounded_descriptor_tables /Fo EquiRectToCubemapCS.cso EquiRectToCubemap.hlsl