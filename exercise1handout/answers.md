# Práctica 01 Respuestas

He puesto la palabra REVIEW en el código para localizar las zonas donde he modificado código.
He modificado los archivos `exercise1.h` y `lineshapes.cpp`.

```
Al cambiar los valores de la posición de la cámara, ¿sobre qué eje [x,y o z] tienes que actuar en cada caso?
```

El eje Y es responsable del movimiento vertical, mientras que X y Z son responsables de la traslación a través el plano horizontal.

```
¿Por qué se multiplica cameraPosition por -1.f?
```

Esta multiplicación tiene dos efectos. Por un lado convierte la posición inicial en negativa, permitiendonos ver el cubo y ejes.

Por otra parte, nos permite controlar la cámara de una manera más intuitiva. Por ejemplo, sumar en la Y implica la elevación de la cámara sobre el 0,0 gracias a ese -1.

```
¿Cómo se llama la matriz de la cámara en el shader? ¿Para qué se utiliza en el shader?
```

Se usa de esta manera `proj * view * model * vec4 (vertex_position, 1.0);`

Lo que está ocurriendo aquí es una multiplicación de 3 matrices con la posición de un vértice.

La primera matriz contiene los datos correspondientes al plano de proyeción de la cámara. La segunda contiene su posición y rotación en el mundo. De la tercera no tengo ni la menor idea.

El resultado de esta operación es una posición en la ventana del ejecutable.

```
Práctica de las aristas
```

Está todo descrito en lineshapes.cpp.
