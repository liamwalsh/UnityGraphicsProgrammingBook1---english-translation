= @<raw>{MultiPlane\nPerspectiveProjection}

In this chapter, we will introduce an image projection method that allows you to project an image on multiple surfaces such as the walls and floors of a rectangular parallelepiped room with a projector and experience the experience of being in the CG world. In addition, I will explain the processing of the camera in CG and its application examples as the background.
Please see the sample project in Assets/RoomProjection in Unity Project @<fn>{project} of Unity Graphics Programming.
In addition, this content is "Math Seminar December 2016 Issue"@<fn>{susemi} Based on the content contributed to.
//footnote[project][Sample project https://github.com/IndieVisualLab/UnityGraphicsProgramming]
//footnote[project][LIAM PUT ENGLISH TRANSLATION PROJECT HERE]
//footnote[susemi][https://www.nippyo.co.jp/shop/magazine/7292.html]



== Mechanism of camera in CG
The camera process in general CG is a process of projecting a visible 3D model to a 2D image using perspective projection conversion.
The perspective projection transformation is a local coordinate system whose origin is the center of each model, a world coordinate system whose origin is a uniquely determined location in the CG world, a view coordinate system centered on the camera, and a clip coordinate system for clipping (this Is a four-dimensional coordinate system in which w is also meaningful, and a three-dimensional one is called @<kw>{NDC,Normalized Device Coordinates}, which represents the two-dimensional position of the output screen. Project the coordinates of the vertices in the order of screen coordinate system.

//image[spaces][Flow of coordinate conversion]

Also, since each of these transformations can be represented by one matrix, it is often practiced to multiply the matrices in advance so that several coordinate transformations can be done only once by multiplying the matrix and the vector.

== Perspective consistency with multiple cameras
With a camera in CG, a quadrangular pyramid whose head vertex is located at the camera position and whose bottom is aligned with the camera's orientation is called a viewing frustum, and can be illustrated as a 3D volume representing the projection of the camera.

//image[frustum][視錐台][scale=0.5]

２つのカメラの視錐台が頭頂点を共有し側面が接していれば、投影面が別々の方向を向いていても映像的には繋がり、かつ、頭頂点から見たときのパースペクティブが一致します。

//image[frustum2][接する視錐台（わかりやすいように少し離して配置しています）]

これは視錐台を無数の視線の集合とみなすことで「視線同士が連続している（＝パースペクティブ上矛盾のない映像を投影することができる）」と考えることで理解できます。
この考えを５つのカメラまで拡張し、５つの視錐台が頭頂点を共有しそれぞれ隣接する視錐台と接するような配置になるよう画角を調整することで、部屋の各面に対応した映像を生成することができます。理論上は天井も含めた６面も可能ですが今回はプロジェクタの設置スペースとして考え、天井を除く５面を想定しています。

//image[frustum5][部屋に対応した５つの視錐台]

この頭頂点、つまり全てのカメラの位置に相当する場所から鑑賞することで、部屋どの方向を見てもパースペクティブ上矛盾のない映像を鑑賞することができます。


== プロジェクション行列の導出
プロジェクション行列（以下@<m>{Proj}）はビュー座標系からクリップ座標系へ変換する行列です。

 * @<m>{C}: クリップ座標系おける位置ベクトル
 * @<m>{V}: をビュー座標系における位置ベクトル

 として式で表すと以下のようになります。
//texequation{
C = Proj * V
//}

さらに@<m>{C}の各要素を@<m>{C_{w\}}で除算することでNDCでの位置座標となります。

//texequation{
NDC = (\frac{C_{x}}{C_{w}},\frac{C_{y}}{C_{w}},\frac{C_{z}}{C_{w}})
//}

なお、@<m>{C_{w\}=-V_{z\}}とな（るように@<m>{Proj}を作）ります。ビュー座標系の正面方向がZマイナス方向なのでマイナスがかかっています。
NDCでは表示範囲を@<m>{-1\leq x,y,z\leq 1}とし、この変換で@<m>{V_{z\}}に応じて@<m>{V_{x,y\}}が拡大縮小することにより遠近法の表現が得られます。

それでは、@<m>{Proj}をどのように作ればよいか考えてみましょう。
ビュー座標系におけるnearClipPlaneの右上の点の座標を@<m>{N}、farClipPlaneの右上の点の座標を@<m>{F}としておきます。

//image[NF][N,F][scale=0.5]

まずは@<m>{x}に注目してみると、

 * 投影範囲が@<m>{-1\leq x\leq 1}になること
 * あとで@<m>{C_{w\}(=-V_{z\})}で除算されること

を考慮すると
//texequation{
Proj[0,0] = \frac{N_{z}}{N_{x}}
//}
とすれば良さそうです。
x,zの比率は変わらないので@<m>{Proj[0][0] = \frac{F_{z\}\}{F_{x\}\}}など視錐台の右端ならどのx,zでも構いません。

同様に
//texequation{
Proj[1,1] = \frac{N_{z}}{N_{y}}
//}
も求まります。

@<m>{z}については少し工夫が必要です。
@<m>{Proj * V}でｚに関わる計算は以下ようになります。

//texequation{
C_{z} = Proj[2,2]*V_{z} + Proj[2,3]*V_{w}　(ただし、V_{w} = 1)
//}
//texequation{
NDC_{z} = \frac{C_{z}}{C_{w}}（ただし、C_{w} = -V_{z}）
//}


ここで、@<m>{N_{z\} → -1, F_{z\} → 1}と変換したいので、@<m>{a = Proj[2,2], b = Proj[2,3]} と置いて

//texequation{
-1 = \frac{1}{N_{z}} (aN_{z} +b),　
1 = \frac{1}{F_{z}} (aF_{z} +b)
//}

この連立方程式から解が得られます。

//texequation{
Proj[2,2] = a = \frac{F_{z}+N_{z}}{F_{z}-N_{z}},　
Proj[2,3] = b = \frac{-2F_{z}N_{z}}{F_{z}-N_{z}}
//}

また、@<m>{C_{w\} = -V_{w\}}となるようにしたいので

//texequation{
Proj[3,2] = -1
//}
とします。

したがって求める@<m>{Proj}は以下の形になります。

//texequation{
Proj = \left(
\begin{array}{cccc}
    \frac{N_{z}}{N_{x}} &   0 & 0 & 0\\
    0   & \frac{N_{z}}{N_{y}} & 0 & 0\\
    0   &   0 & \frac{F_{z}+N_{z}}{F_{z}-N_{z}} & \frac{-2F_{z}N_{z}}{F_{z}-N_{z}} \\
    0   &   0 & -1 & 0
\end{array}
\right)
//}


=== Camera.projectionMatrix の罠
シェーダー内などでプロジェクション行列を扱ったことがある方の中にはもしかしたらここまでの内容に違和感を持つ方もいらっしゃるかもしれません。
実はUnityのプロジェクション行列の扱いはややこしく、ここまでの内容はCamera.projectionMatrixについての解説になります。
この値はプラットフォームによらずOpenGLに準拠しています@<fn>{opengl_projection}。@<m>{-1\leq NDC_{z\}\leq 1}や @<m>{C_{w\} = -V_{w\}}となるのもこのためです。
//footnote[opengl_projection][https://docs.unity3d.com/ScriptReference/GL.GetGPUProjectionMatrix.html]

しかしUnity内でシェーダーに渡す際にプラットフォームに依存した形に変換するため、Camera.projectionMatrixをそのまま透視投影変換に使っているとは限りません。とくに@<m>{NDC_{z\}}の範囲や向き（つまりZバッファの扱い）は多様でひっかかりやすいポイントになっています@<fn>{zbuff}。
//footnote[zbuff][https://docs.unity3d.com/Manual/SL-PlatformDifferences.html]


== 視錐台の操作
=== 投影面のサイズ合わせ
視錐台の底面つまり投影面の形はカメラの@<kw>{fov,fieldOfView,画角}と@<kw>{aspect,アスペクト比}に依存しています。
Unityのカメラでは画角はInspectorで公開されているものの、アスペクト比は公開されていないのでコードから編集する必要があります。
@<kw>{faceSize,部屋の面のサイズ}、@<kw>{distance,視点から面までの距離}から画角とアスペクト比を求めるコードは以下のようになります。

//list[fov_aspect][画角とアスペクト比を求める][csharp]{
camera.aspect = faceSize.x / faceSize.y;
camera.fieldOfView = 2f * Mathf.Atan2(faceSize.y * 0.5f, distance) 
                     * Mathf.Rad2Deg;
//}

Mathf.Atan2() でfovの半分の角度をradianで求め、２倍し、Camera.fieldOfViewに代入するためdegreeに直している点に注意して下さい。

=== 投影面の移動（レンズシフト）
視点が部屋の中心にない場合も考慮してみましょう。
視点に対して投影面が上下左右に平行移動することができれば、投影面に対して視点が移動したことと同じ効果が得られます。
これは現実世界ではプロジェクターなどで映像の投影位置を調整する@<kw>{レンズシフト}という機能に相当します。

//image[lensshift][レンズシフト]

あらためてカメラが透視投影する仕組みに立ち返ってみるとレンズシフトはどの部分で行う処理になるでしょうか？
プロジェクション行列でNDCに射影する際に、x,yをずらせば良さそうです
もう一度Projection行列を見てみましょう。

//texequation{
Proj = \left(
\begin{array}{cccc}
    \frac{N_{z}}{N_{x}} &   0 & 0 & 0\\
    0   & \frac{N_{z}}{N_{y}} & 0 & 0\\
    0   &   0 & \frac{F_{z}+N_{z}}{F_{z}-N_{z}} & \frac{-2F_{z}N_{z}}{F_{z}-N_{z}} \\
    0   &   0 & -1 & 0
\end{array}
\right)
//}

@<m>{C_{x\},C_{y\}}がずれればいいので、行列の平行移動成分である@<m>{Proj[0,3],Pro[1,3]}になにか入れたくなりますが、あとで@<m>{C_{w\}}で除算されることを考慮すると、@<m>{Proj[0,2],Pro[1,2]}に入れるのが正解です。

//texequation{
Proj = \left(
\begin{array}{cccc}
    \frac{N_{z}}{N_{x}} &   0 & LensShift_{x} & 0\\
    0   & \frac{N_{z}}{N_{y}} & LensShift_{y} & 0\\
    0   &   0 & \frac{F_{z}+N_{z}}{F_{z}-N_{z}} & \frac{-2F_{z}N_{z}}{F_{z}-N_{z}} \\
    0   &   0 & -1 & 0
\end{array}
\right)
//}

LensShiftの単位はNDCですので投影面のサイズを-1〜1に正規化したものになります。コードにすると以下のようになります。

//list[Lensshift][レンズシフトをプロジェクション行列に反映][csharp]{
var shift = new Vector2(
    positionOffset.x / faceSize.x,
    positionOffset.y / faceSize.y
) * 2f;
var projectionMatrix = camera.projectionMatrix;
projectionMatrix[0,2] = shift.x;
projectionMatrix[1,2] = shift.y;
camera.projectionMatrix = projectionMatrix;
//}

一度Camera.projectionMatrixにsetするとCamera.ResetProjectionMatrix()を呼ばない限りその後のCamera.fieldOfViewの変更が反映されなくなる点に注意が必要です。@<fn>{resetProjectionMatrix}
//footnote[resetProjectionMatrix][https://docs.unity3d.com/ScriptReference/Camera-projectionMatrix.html]


== 部屋プロジェクション
直方体の部屋で、鑑賞者の視点位置をトラッキングできているものとします。
前節の方法で視錐台の投影面のサイズと平行移動ができるので、視点位置を視錐台の頭頂点、壁面や床面を投影面としたときその形状に合うような視錐台を動的に求める事ができます。各カメラをこのような視錐台にすることによって各投影面用の映像を作ることができます。この映像を実際の部屋に投影すれは鑑賞者からはパースのあったCG世界が見えるようになります。

//image[projection_room][部屋のシミュレーション（俯瞰視点）]
//image[projection_firstperson][部屋のシミュレーション（一人称視点）]


== まとめ
本章ではプロジェクション行列を応用することで複数の投影面でパースを合わせる投影方法を紹介しました。目の前にディスプレイを置くのではなく視界の広い範囲を動的に反応する映像にしてしまう点で、昨今のHMD型と似て非なるアプローチのVRと言えるのではないかと思います。
また、この方法では両眼視差や目のフォーカスを騙せるわけではないのでそのままでは立体視できずに「壁に投影された動く絵」に見えてしまう可能性があります。没入感を高めるためにはもう少し工夫する必要がありそうです。

 * 両眼視差が小さくなるように部屋を大きくして鑑賞者から投影面までの距離を遠くする
 * 反射光などで投影面の平面が意識されてしまうことをできるだけ防ぐ
 ** 暗めの映像にする
 ** 壁や床をできるだけ鏡面反射しない素材にする

 なお、同様の手法を立体視と組み合わせる「CAVE」@<fn>{cave}という仕組みが知られています。
//footnote[cave][https://en.wikipedia.org/wiki/Cave_automatic_virtual_environment]
