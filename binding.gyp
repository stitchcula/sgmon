{
  "targets": [
    {
      "target_name": "sgmon",
      "sources": [ "addon/sgmon.cc" ],
      "include_dirs":[
        "./addon/include"
      ],
      "libraries":[
        "-pthread ",
        "-ldl ",
        "-L../addon/lib -lstatgrab -Wl,-rpath,'$$ORIGIN'/../../addon/lib -Wl,-z,origin"
      ]
    }
  ]
}
